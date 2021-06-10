#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/fs/cached_dirent.h>
#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/procfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/block.h>
#include <kernel/hal/hw_device.h>
#include <kernel/hal/hw_timer.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/net/arp.h>
#include <kernel/net/interface.h>
#include <kernel/net/neighbor_cache.h>
#include <kernel/net/socket.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/stats.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/init.h>
#include <kernel/util/spinlock.h>

#define PROCFS_ENSURE_ALIGNMENT __attribute__((optimize("align-functions=4")))

#define PROCFS_DEVICE_ID 4

#define PROCFS_FILE_MODE      (S_IFREG | 0444)
#define PROCFS_DIRECTORY_MODE (S_IFDIR | 0555)
#define PROCFS_SYMLINK_MODE   (S_IFLNK | 0777)

#define __PROCFS_IS(data, flag)   ((((uintptr_t)(data)->function)) & (flag))
#define __PROCFS_MAKE(data, flag) ((data)->function = (void *) ((((uintptr_t)(data)->function)) | (flag)))

#define PROCFS_DYNAMIC_FLAG       (1UL)
#define PROCFS_IS_DYNAMIC(data)   __PROCFS_IS(data, PROCFS_DYNAMIC_FLAG)
#define PROCFS_MAKE_DYNAMIC(data) __PROCFS_MAKE(data, PROCFS_DYNAMIC_FLAG)

#define PROCFS_LOADED_FLAG       (2UL)
#define PROCFS_IS_LOADED(data)   __PROCFS_IS(data, PROCFS_LOADED_FLAG)
#define PROCFS_MAKE_LOADED(data) __PROCFS_MAKE(data, PROCFS_LOADED_FLAG)

#define PROCFS_GET_FILE_FUNCTION(data) ((procfs_file_function_t)(((uintptr_t)((data)->function)) & ~(PROCFS_DYNAMIC_FLAG)))
#define PROCFS_GET_DIRECTORY_FUNCTION(data) \
    ((procfs_directory_function_t)(((uintptr_t)((data)->function)) & ~(PROCFS_DYNAMIC_FLAG | PROCFS_LOADED_FLAG)))

static struct file_system fs;
static struct super_block super_block;

static spinlock_t inode_counter_lock = SPINLOCK_INITIALIZER;
static ino_t inode_counter = 1;

static struct inode *root;

static struct file_system fs = {
    .name = "procfs",
    .mount = &procfs_mount,
};

static struct inode_operations procfs_i_op = {
    .lookup = &procfs_lookup,
    .open = &procfs_open,
    .read_all = &procfs_read_all,
};

static struct inode_operations procfs_dir_i_op = {
    .lookup = &procfs_lookup,
    .open = &procfs_open,
};

static struct file_operations procfs_f_op = {
    .read = &procfs_read,
    .poll = inode_poll,
    .poll_finish = inode_poll_finish,
};

static struct file_operations procfs_dir_f_op = {
    .poll = inode_poll,
    .poll_finish = inode_poll_finish,
};

static struct inode *procfs_create_inode(mode_t mode, uid_t uid, gid_t gid, struct process *process, void *function) {
    struct inode *inode = calloc(1, sizeof(struct inode) + sizeof(struct procfs_data));
    inode->flags = fs_mode_to_flags(mode);
    inode->super_block = &super_block;
    inode->access_time = inode->modify_time = inode->change_time = time_read_clock(CLOCK_REALTIME);
    inode->fsid = PROCFS_DEVICE_ID;
    inode->gid = gid;
    inode->uid = uid;
    inode->mode = mode;
    inode->i_op = S_ISDIR(mode) ? &procfs_dir_i_op : &procfs_i_op;
    inode->ref_count = 1;
    init_mutex(&inode->lock);

    spin_lock(&inode_counter_lock);
    inode->index = inode_counter++;
    spin_unlock(&inode_counter_lock);

    inode->private_data = inode + 1;
    struct procfs_data *data = inode->private_data;
    data->function = function;
    data->fd = -1;
    data->process = process;

    return inode;
}

static struct process *procfs_get_process(struct inode *inode) {
    struct procfs_data *data = inode->private_data;
    if (PROCFS_IS_DYNAMIC(data)) {
        return get_current_task()->process;
    }

    return data->process;
}

static struct procfs_buffer procfs_get_data(struct inode *inode, bool need_buffer) {
    struct procfs_data *data = inode->private_data;
    struct process *process = procfs_get_process(inode);

    procfs_file_function_t getter = PROCFS_GET_FILE_FUNCTION(data);
    return getter(data, process, need_buffer);
}

static void procfs_cleanup_data(struct procfs_buffer data, struct inode *inode __attribute__((unused))) {
    free(data.buffer);
}

static void procfs_refresh_directory(struct inode *inode) {
    struct procfs_data *data = inode->private_data;
    struct process *process = procfs_get_process(inode);

    procfs_directory_function_t refresher = PROCFS_GET_DIRECTORY_FUNCTION(data);

    refresher(inode, process, PROCFS_IS_LOADED(data));
    PROCFS_MAKE_LOADED(data);
}

struct inode *procfs_lookup(struct inode *inode, const char *name) {
    if (!(inode->flags & FS_DIR)) {
        struct procfs_buffer result = procfs_get_data(inode, false);
        inode->size = result.size;
        procfs_cleanup_data(result, inode);
    } else {
        procfs_refresh_directory(inode);
    }

    if (name == NULL) {
        return NULL;
    }

    return fs_lookup_in_cache(inode->dirent_cache, name);
}

struct file *procfs_open(struct inode *inode, int flags, int *error) {
    assert(!(flags & O_RDWR));

    *error = 0;
    return fs_create_file(inode, inode->flags, 0, flags, (inode->flags & FS_DIR) ? &procfs_dir_f_op : &procfs_f_op, NULL);
}

int procfs_read_all(struct inode *inode, void *buffer) {
    struct procfs_buffer data = procfs_get_data(inode, true);

    memcpy(buffer, data.buffer, data.size);

    procfs_cleanup_data(data, inode);
    return 0;
}

ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct inode *inode = fs_file_inode(file);
    assert(inode);

    struct procfs_buffer data = procfs_get_data(inode, true);

    size_t nread = fs_do_read(buffer, offset, len, data.buffer, data.size);

    procfs_cleanup_data(data, inode);
    return nread;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_cwd(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                               bool need_buffer) {
    if (!process->cwd) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    char *buffer = get_tnode_path(process->cwd);
    size_t length = strlen(buffer);
    if (!need_buffer) {
        free(buffer);
        buffer = NULL;
    }
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_exe(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                               bool need_buffer) {
    if (!process->exe) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    char *buffer = get_tnode_path(process->exe);
    size_t length = strlen(buffer);
    if (!need_buffer) {
        free(buffer);
        buffer = NULL;
    }
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_status(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                                  bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    char tty_string[16];
    if (process->tty != -1) {
        snprintf(tty_string, sizeof(tty_string) - 1, "/dev/tty%d", process->tty);
    } else {
        snprintf(tty_string, sizeof(tty_string) - 1, "%s", "?");
    }

    struct task *main_task = find_by_tid(process->pgid, process->main_tid);
    size_t length =
        snprintf(buffer, need_buffer ? PAGE_SIZE : 0,
                 "NAME: %s\n"
                 "PID: %d\n"
                 "STATE: %s\n"
                 "UID: %hu\n"
                 "GID: %hu\n"
                 "PPID: %d\n"
                 "UMASK: %04o\n"
                 "EUID: %hu\n"
                 "EGID: %hu\n"
                 "PGID: %d\n"
                 "SID: %d\n"
                 "TTY: %s\n"
                 "PRIORITY: %d\n"
                 "NICE: %d\n"
                 "VIRTUAL_MEMORY: %lu\n"
                 "RESIDENT_MEMORY: %lu\n"
                 "START_TIME.tv_sec: %lu\n"
                 "START_TIME.tv_nsec: %lu\n",
                 process->name, process->pid, main_task ? task_state_to_string(main_task->sched_state) : "? (unknown)", process->uid,
                 process->gid, proc_getppid(process), process->umask, process->euid, process->egid, process->pgid, process->sid, tty_string,
                 process->priority, process->priority - PROCESS_DEFAULT_PRIORITY, vm_compute_total_virtual_memory(process),
                 process->resident_memory, process->start_time.tv_sec, process->start_time.tv_nsec);
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_stack(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                                 bool need_buffer) {
    struct task *main_task = find_by_tid(process->pgid, process->main_tid);
    if (!main_task) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    size_t buffer_max = need_buffer ? PAGE_SIZE : 0;
    char *buffer = malloc(buffer_max);
    size_t length = kernel_stack_trace_for_procfs(main_task, buffer, buffer_max);
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_vm(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                              bool need_buffer) {
    char *buffer = need_buffer ? malloc(2 * PAGE_SIZE) : NULL;
    size_t length = 0;
    struct vm_region *vm = process->process_memory;
    while (vm) {
        length += snprintf(buffer + length, need_buffer ? 2 * PAGE_SIZE - length : 0,
                           "START: %p END: %p TYPE: %s\n"
                           "PERM: %c%c%c%c BACKED: %s\n",
                           (void *) vm->start, (void *) vm->end, vm_type_to_string(vm->type), (vm->flags & VM_PROT_NONE) ? ' ' : 'r',
                           (vm->flags & VM_WRITE) ? 'w' : ' ', (vm->flags & VM_NO_EXEC) ? ' ' : 'x', (vm->flags & VM_STACK) ? 's' : ' ',
                           vm->vm_object ? "yes" : "no");
        vm = vm->next;
    }

    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_signal(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                                  bool need_buffer) {
    char *buffer = need_buffer ? malloc(2 * PAGE_SIZE) : NULL;
    size_t length = 0;
    for (int i = 1; i < _NSIG; i++) {
        length +=
            snprintf(buffer + length, need_buffer ? 2 * PAGE_SIZE - length : 0,
                     "SIGNAL: %d (%s)\n"
                     "STATE: %s\n"
                     "FLAGS: %c%c%c%c%c%c%c\n"
                     "MASK: %#.16lX\n",
                     i, strsignal(i),
                     process->sig_state[i].sa_handler == SIG_IGN   ? "ignored"
                     : process->sig_state[i].sa_handler == SIG_DFL ? "default"
                                                                   : "handled",
                     process->sig_state[i].sa_flags & SA_NOCLDSTOP ? 'C' : ' ', process->sig_state[i].sa_flags & SA_ONSTACK ? 'S' : ' ',
                     process->sig_state[i].sa_flags & SA_RESETHAND ? 'X' : ' ', process->sig_state[i].sa_flags & SA_RESTART ? 'R' : ' ',
                     process->sig_state[i].sa_flags & SA_SIGINFO ? 'I' : ' ', process->sig_state[i].sa_flags & SA_NOCLDWAIT ? 'W' : ' ',
                     process->sig_state[i].sa_flags & SA_NODEFER ? 'D' : ' ', process->sig_state[i].sa_mask);
    }
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_cmdline(struct procfs_data *data __attribute__((unused)),
                                                                   struct process *process, bool need_buffer) {
    if (!process->args_context) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    size_t length = process->args_context->args_bytes;
    if (!need_buffer) {
        return (struct procfs_buffer) { NULL, length };
    }

    char *buffer = malloc(length);
    memcpy(buffer, process->args_context->prepend_args_buffer, length);
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_environ(struct procfs_data *data __attribute__((unused)),
                                                                   struct process *process, bool need_buffer) {
    if (!process->args_context) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    size_t length = process->args_context->env_bytes;
    if (!need_buffer) {
        return (struct procfs_buffer) { NULL, length };
    }

    char *buffer = malloc(length);
    memcpy(buffer, process->args_context->env_buffer, length);
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_pid_sched(struct procfs_data *data __attribute__((unused)),
                                                                     struct process *process, bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    size_t length = snprintf(buffer, need_buffer ? PAGE_SIZE : 0,
                             "USER_TICKS: %lu\n"
                             "KERNEL_TICKS: %lu\n",
                             process->rusage_self.ru_utime.tv_sec * 1000 + process->rusage_self.ru_utime.tv_usec / 1000,
                             process->rusage_self.ru_stime.tv_sec * 1000 + process->rusage_self.ru_stime.tv_usec / 1000);
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_fd(struct procfs_data *data, struct process *process, bool need_buffer) {
    int fd = data->fd;
    struct file *file = process->files[fd].file;
    if (!file) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    struct tnode *fd_tnode = file->tnode;
    if (!fd_tnode) {
        // This should be NULL for things like unamed FIFO's or sockets.
        return (struct procfs_buffer) { NULL, 0 };
    }

    char *buffer = get_tnode_path(fd_tnode);
    size_t length = strlen(buffer);
    if (!need_buffer) {
        free(buffer);
        buffer = NULL;
    }
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static void procfs_create_fd_directory_structure(struct inode *parent, struct process *process, bool loaded) {
    if (!loaded) {
        parent->dirent_cache = fs_create_dirent_cache();
    }

    for (int i = 0; i < FOPEN_MAX; i++) {
        char fd_string[32];
        size_t length = snprintf(fd_string, sizeof(fd_string) - 1, "%d", i);

        struct file *file = process->files[i].file;
        struct inode *existing_inode = fs_lookup_in_cache(parent->dirent_cache, fd_string);
        if (existing_inode && !file) {
            fs_del_dirent_cache(parent->dirent_cache, fd_string);
            drop_inode_reference(existing_inode);
        } else if (!existing_inode && file) {
            struct inode *inode = procfs_create_inode(PROCFS_SYMLINK_MODE, process->uid, process->gid, process, procfs_fd);
            struct procfs_data *data = inode->private_data;
            data->fd = i;

            fs_put_dirent_cache(parent->dirent_cache, inode, fd_string, length);
        }
    }
}

PROCFS_ENSURE_ALIGNMENT static void procfs_create_process_directory_structure(struct inode *parent, struct process *process, bool loaded) {
    if (!loaded) {
        parent->dirent_cache = fs_create_dirent_cache();

        struct inode *status_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_status);
        fs_put_dirent_cache(parent->dirent_cache, status_inode, "status", strlen("status"));

        struct inode *stack_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_stack);
        fs_put_dirent_cache(parent->dirent_cache, stack_inode, "stack", strlen("stack"));

        struct inode *cwd_inode = procfs_create_inode(PROCFS_SYMLINK_MODE, process->uid, process->gid, process, procfs_cwd);
        fs_put_dirent_cache(parent->dirent_cache, cwd_inode, "cwd", strlen("cwd"));

        struct inode *exe_inode = procfs_create_inode(PROCFS_SYMLINK_MODE, process->uid, process->gid, process, procfs_exe);
        fs_put_dirent_cache(parent->dirent_cache, exe_inode, "exe", strlen("exe"));

        struct inode *vm_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_vm);
        fs_put_dirent_cache(parent->dirent_cache, vm_inode, "vm", strlen("vm"));

        struct inode *signal_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_signal);
        fs_put_dirent_cache(parent->dirent_cache, signal_inode, "signal", strlen("signal"));

        struct inode *cmdline_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_cmdline);
        fs_put_dirent_cache(parent->dirent_cache, cmdline_inode, "cmdline", strlen("cmdline"));

        struct inode *environ_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_environ);
        fs_put_dirent_cache(parent->dirent_cache, environ_inode, "environ", strlen("environ"));

        struct inode *sched_inode = procfs_create_inode(PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_pid_sched);
        fs_put_dirent_cache(parent->dirent_cache, sched_inode, "sched", strlen("sched"));

        struct inode *fd_inode =
            procfs_create_inode(PROCFS_DIRECTORY_MODE, process->uid, process->gid, process, procfs_create_fd_directory_structure);
        fs_put_dirent_cache(parent->dirent_cache, fd_inode, "fd", strlen("fd"));
    }
}

static void procfs_destroy_process_directory_structure(struct inode *parent);

static void procfs_do_destroy_directory(struct hash_entry *cached_dirent, void *parent __attribute__((unused))) {
    struct cached_dirent *dirent = hash_table_entry(cached_dirent, struct cached_dirent);
    struct inode *inode = dirent->inode;

    if (inode->flags & FS_DIR) {
        procfs_destroy_process_directory_structure(inode);
    }
    drop_inode_reference(inode);
}

static void procfs_destroy_process_directory_structure(struct inode *parent) {
    if (parent->dirent_cache) {
        fs_dirent_cache_for_each(parent->dirent_cache, procfs_do_destroy_directory, parent);
    }
}

void procfs_register_process(struct process *process) {
    assert(root);

    struct inode *inode =
        procfs_create_inode(PROCFS_DIRECTORY_MODE, process->uid, process->gid, process, procfs_create_process_directory_structure);

    char pid_string[16];
    size_t length = snprintf(pid_string, sizeof(pid_string) - 1, "%d", process->pid);

    mutex_lock(&root->lock);
    fs_put_dirent_cache(root->dirent_cache, inode, pid_string, length);
    mutex_unlock(&root->lock);
}

void procfs_unregister_process(struct process *process) {
    assert(root);

    char pid_string[16];
    snprintf(pid_string, sizeof(pid_string) - 1, "%d", process->pid);
    struct inode *inode = fs_lookup_in_cache(root->dirent_cache, pid_string);
    assert(inode);

    fs_del_dirent_cache(root->dirent_cache, pid_string);
    procfs_destroy_process_directory_structure(inode);
    drop_inode_reference(inode);
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_self(struct procfs_data *data __attribute__((unused)), struct process *process,
                                                                bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    size_t length = snprintf(buffer, need_buffer ? PAGE_SIZE : 0, "%d", process->pid);
    return (struct procfs_buffer) { buffer, length };
}

static void do_procfs_devtree(struct procfs_buffer *buffer, struct hw_device *device, char *aux_buffer, size_t aux_buffer_length,
                              int level) {
    show_hw_device(device, aux_buffer, aux_buffer_length);
    buffer->size += snprintf(buffer->buffer ? buffer->buffer + buffer->size : NULL, buffer->buffer ? PAGE_SIZE - buffer->size : 0, "%*s",
                             2 * level, "");
    buffer->size +=
        snprintf(buffer->buffer ? buffer->buffer + buffer->size : NULL, buffer->buffer ? PAGE_SIZE - buffer->size : 0, "%s\n", aux_buffer);

    spin_lock(&device->tree_lock);
    list_for_each_entry(&device->children, child, struct hw_device, siblings) {
        do_procfs_devtree(buffer, child, aux_buffer, aux_buffer_length, level + 1);
    }
    spin_unlock(&device->tree_lock);
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_devtree(struct procfs_data *data __attribute__((unused)),
                                                                   struct process *process __attribute__((unused)), bool need_buffer) {
    struct procfs_buffer buffer = { .buffer = need_buffer ? malloc(PAGE_SIZE) : NULL, .size = 0 };
    char aux_buffer[256];
    do_procfs_devtree(&buffer, root_hw_device(), aux_buffer, sizeof(aux_buffer), 0);
    return buffer;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_hwtimers(struct procfs_data *data __attribute__((unused)),
                                                                    struct process *process __attribute__((unused)), bool need_buffer) {
    struct procfs_buffer buffer = { .buffer = need_buffer ? malloc(PAGE_SIZE) : NULL, .size = 0 };
    char aux_buffer[512];
    list_for_each_entry(hw_timers(), hw_timer, struct hw_timer, list) {
        show_hw_timer(hw_timer, aux_buffer, sizeof(aux_buffer));
        buffer.size +=
            snprintf(buffer.buffer ? buffer.buffer + buffer.size : NULL, buffer.buffer ? PAGE_SIZE - buffer.size : 0, "%s\n", aux_buffer);
    }
    return buffer;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_blockdev(struct procfs_data *data __attribute__((unused)),
                                                                    struct process *process __attribute__((unused)), bool need_buffer) {
    struct procfs_buffer buffer = { .buffer = need_buffer ? malloc(PAGE_SIZE) : NULL, .size = 0 };
    char aux_buffer[512];
    block_for_each_device(block_device) {
        block_show_device(block_device, aux_buffer, sizeof(aux_buffer));
        buffer.size +=
            snprintf(buffer.buffer ? buffer.buffer + buffer.size : NULL, buffer.buffer ? PAGE_SIZE - buffer.size : 0, "%s\n", aux_buffer);
    }
    return buffer;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_filesystems(struct procfs_data *data __attribute__((unused)),
                                                                       struct process *process __attribute__((unused)), bool need_buffer) {
    struct procfs_buffer buffer = { .buffer = need_buffer ? malloc(PAGE_SIZE) : NULL, .size = 0 };
    char aux_buffer[256];
    fs_for_each_file_system(fs) {
        fs_show_file_system(fs, aux_buffer, sizeof(aux_buffer));
        buffer.size +=
            snprintf(buffer.buffer ? buffer.buffer + buffer.size : NULL, buffer.buffer ? PAGE_SIZE - buffer.size : 0, "%s\n", aux_buffer);
    }
    return buffer;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_sched(struct procfs_data *data __attribute((unused)),
                                                                 struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    size_t length = snprintf(buffer, need_buffer ? PAGE_SIZE : 0,
                             "IDLE_TICKS: %lu\n"
                             "USER_TICKS: %lu\n"
                             "KERNEL_TICKS: %lu\n",
                             sched_idle_ticks(), sched_user_ticks(), sched_kernel_ticks());
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_meminfo(struct procfs_data *data __attribute__((unused)),
                                                                   struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    size_t length =
        snprintf(buffer, need_buffer ? PAGE_SIZE : 0,
                 "ALLOCATED_MEMORY: %lu\n"
                 "TOTAL_MEMORY: %lu\n"
                 "MAX_MEMORY: %lu\n",
                 g_phys_page_stats.phys_memory_allocated, g_phys_page_stats.phys_memory_total, g_phys_page_stats.phys_memory_max);
    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_cpus(struct procfs_data *data __attribute__((unused)),
                                                                struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    size_t length = 0;
    struct processor *processor = get_processor_list();
    while (processor) {
        length += snprintf(buffer + length, need_buffer ? PAGE_SIZE - length : 0,
                           "ID: %d\n"
                           "LOCAL APIC ID: %u\n"
                           "ACPI ID: %u\n"
                           "ENABLED: %s\n"
                           "IDLE TASK: %d:%d\n",
                           processor->id, processor->arch_processor.local_apic_id, processor->arch_processor.acpi_id,
                           processor->enabled ? "yes" : "no", processor->idle_task->process->pid, processor->idle_task->tid);
        processor = processor->next;
    }

    return (struct procfs_buffer) { buffer, length };
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_kheap(struct procfs_data *data __attribute((unused)),
                                                                 struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;
    size_t length =
        snprintf(buffer, need_buffer ? PAGE_SIZE : 0,
                 "ALLOC: %lu\n"
                 "FREE: %lu\n"
                 "DELTA: %lu\n",
                 g_kmalloc_stats.alloc_count, g_kmalloc_stats.free_count, g_kmalloc_stats.alloc_count - g_kmalloc_stats.free_count);
    return (struct procfs_buffer) { buffer, length };
}

static void arp_for_each(struct hash_entry *_neighbor, void *_buf) {
    struct neighbor_cache_entry *neighbor = hash_table_entry(_neighbor, struct neighbor_cache_entry);
    struct procfs_buffer *buf = _buf;
    buf->size +=
        snprintf(buf->buffer + buf->size, buf->buffer ? PAGE_SIZE - buf->size : 0, "[%s]: %u.%u.%u.%u => %02x:%02x:%02x:%02x:%02x:%02x\n",
                 neighbor->key.interface->name, neighbor->key.ip_v4_address.addr[0], neighbor->key.ip_v4_address.addr[1],
                 neighbor->key.ip_v4_address.addr[2], neighbor->key.ip_v4_address.addr[3], neighbor->link_layer_address.addr[0],
                 neighbor->link_layer_address.addr[1], neighbor->link_layer_address.addr[2], neighbor->link_layer_address.addr[3],
                 neighbor->link_layer_address.addr[4], neighbor->link_layer_address.addr[5]);
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_arp(struct procfs_data *data __attribute__((unused)),
                                                               struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;

    struct procfs_buffer buf = { buffer, 0 };

    struct hash_map *map = net_neighbor_cache();
    hash_for_each(map, arp_for_each, &buf);
    return buf;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_interfaces(struct procfs_data *data __attribute__((unused)),
                                                                      struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;

    struct procfs_buffer buf = { buffer, 0 };
    net_for_each_interface(interface) {
        struct link_layer_address mac_address = interface->link_layer_address;
        buf.size += snprintf(buf.buffer + buf.size, buf.buffer ? PAGE_SIZE - buf.size : 0,
                             "NAME: %s\n"
                             "INTERFACE IP: %d.%d.%d.%d\n"
                             "DEFAULT GATEWAY IP: %d.%d.%d.%d\n"
                             "SUBNET MASK: %d.%d.%d.%d\n"
                             "MAC ADDRESS: %02x:%02x:%02x:%02x:%02x:%02x\n",
                             interface->name, interface->address.addr[0], interface->address.addr[1], interface->address.addr[2],
                             interface->address.addr[3], interface->default_gateway.addr[0], interface->default_gateway.addr[1],
                             interface->default_gateway.addr[2], interface->default_gateway.addr[3], interface->mask.addr[0],
                             interface->mask.addr[1], interface->mask.addr[2], interface->mask.addr[3], mac_address.addr[0],
                             mac_address.addr[1], mac_address.addr[2], mac_address.addr[3], mac_address.addr[4], mac_address.addr[5]);
    }
    return buf;
}

PROCFS_ENSURE_ALIGNMENT static struct procfs_buffer procfs_protocols(struct procfs_data *data __attribute__((unused)),
                                                                     struct process *process __attribute__((unused)), bool need_buffer) {
    char *buffer = need_buffer ? malloc(PAGE_SIZE) : NULL;

    struct procfs_buffer buf = { buffer, 0 };
    net_for_each_protocol(protocol) {
        buf.size +=
            snprintf(buf.buffer + buf.size, buf.buffer ? PAGE_SIZE - buf.size : 0,
                     "NAME: %s\n"
                     "DOMAIN: %d\n"
                     "TYPE: %d\n"
                     "PROTOCOL: %d\n"
                     "DEFAULT: %s\n",
                     protocol->name, protocol->domain, protocol->type, protocol->protocol, protocol->is_default_protocol ? "YES" : "NO");
    }
    return buf;
}

PROCFS_ENSURE_ALIGNMENT static void procfs_create_net_directory_structure(struct inode *parent,
                                                                          struct process *process __attribute__((unused)), bool loaded) {
    assert(parent);

    if (!loaded) {
        struct inode *arp_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_arp);
        struct procfs_data *data = arp_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *interfaces_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_interfaces);
        data = interfaces_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *protocols_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_protocols);
        data = protocols_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        mutex_lock(&parent->lock);
        fs_put_dirent_cache(parent->dirent_cache, arp_inode, "arp", strlen("arp"));
        fs_put_dirent_cache(parent->dirent_cache, interfaces_inode, "interfaces", strlen("interfaces"));
        fs_put_dirent_cache(parent->dirent_cache, protocols_inode, "protocols", strlen("protocols"));
        mutex_unlock(&parent->lock);
    }
}

PROCFS_ENSURE_ALIGNMENT static void procfs_create_base_directory_structure(struct inode *parent,
                                                                           struct process *process __attribute__((unused)), bool loaded) {
    assert(parent);

    if (!loaded) {
        struct inode *self_inode = procfs_create_inode(PROCFS_SYMLINK_MODE, 0, 0, NULL, procfs_self);
        struct procfs_data *data = self_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *devtree_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_devtree);
        data = devtree_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *blockdev_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_blockdev);
        data = blockdev_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *hwtimers_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_hwtimers);
        data = hwtimers_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *filesystems_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_filesystems);
        data = filesystems_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *sched_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_sched);
        data = self_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *meminfo_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_meminfo);
        data = meminfo_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *cpus_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_cpus);
        data = cpus_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *kheap_inode = procfs_create_inode(PROCFS_FILE_MODE, 0, 0, NULL, procfs_kheap);
        data = kheap_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct inode *net_directory = procfs_create_inode(PROCFS_DIRECTORY_MODE, 0, 0, NULL, procfs_create_net_directory_structure);
        net_directory->dirent_cache = fs_create_dirent_cache();

        mutex_lock(&parent->lock);
        fs_put_dirent_cache(parent->dirent_cache, blockdev_inode, "blockdev", strlen("blockdev"));
        fs_put_dirent_cache(parent->dirent_cache, cpus_inode, "cpus", strlen("cpus"));
        fs_put_dirent_cache(parent->dirent_cache, devtree_inode, "devtree", strlen("devtree"));
        fs_put_dirent_cache(parent->dirent_cache, filesystems_inode, "filesystems", strlen("filesystems"));
        fs_put_dirent_cache(parent->dirent_cache, hwtimers_inode, "hwtimers", strlen("hwtimers"));
        fs_put_dirent_cache(parent->dirent_cache, self_inode, "self", strlen("self"));
        fs_put_dirent_cache(parent->dirent_cache, sched_inode, "sched", strlen("sched"));
        fs_put_dirent_cache(parent->dirent_cache, kheap_inode, "kheap", strlen("kheap"));
        fs_put_dirent_cache(parent->dirent_cache, meminfo_inode, "meminfo", strlen("meminfo"));
        fs_put_dirent_cache(parent->dirent_cache, net_directory, "net", strlen("net"));
        mutex_unlock(&parent->lock);
    }
}

int procfs_mount(struct block_device *device, unsigned long, const void *, struct super_block **super_block_p) {
    assert(!device);

    *super_block_p = &super_block;

    return 0;
}

static void init_procfs() {
    root = procfs_create_inode(PROCFS_DIRECTORY_MODE, 0, 0, NULL, procfs_create_base_directory_structure);
    root->dirent_cache = fs_create_dirent_cache();
    struct procfs_data *root_data = root->private_data;
    PROCFS_MAKE_DYNAMIC(root_data);

    super_block.fsid = root->fsid;
    super_block.op = NULL;
    super_block.root = root;
    super_block.block_size = PAGE_SIZE;

    register_fs(&fs);
}
INIT_FUNCTION(init_procfs, fs);
