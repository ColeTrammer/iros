#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <kernel/fs/file.h>
#include <kernel/fs/file_system.h>
#include <kernel/fs/inode.h>
#include <kernel/fs/inode_store.h>
#include <kernel/fs/procfs.h>
#include <kernel/fs/super_block.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/spinlock.h>

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
static struct tnode *t_root;

static struct file_system fs = { "procfs", 0, &procfs_mount, NULL, NULL };

static struct inode_operations procfs_i_op = { NULL, &procfs_lookup, &procfs_open,     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                               NULL, NULL,           &procfs_read_all, NULL, NULL };

static struct inode_operations procfs_dir_i_op = { NULL, &procfs_lookup, &procfs_open, NULL, NULL, NULL, NULL, NULL,
                                                   NULL, NULL,           NULL,         NULL, NULL, NULL, NULL, NULL };

static struct file_operations procfs_f_op = { NULL, &procfs_read, NULL, NULL };

static struct file_operations procfs_dir_f_op = { NULL, NULL, NULL, NULL };

static struct inode *procfs_create_inode(struct tnode *tparent, mode_t mode, uid_t uid, gid_t gid, struct process *process,
                                         void *function) {
    struct inode *inode = calloc(1, sizeof(struct inode) + sizeof(struct procfs_data));
    inode->flags = fs_mode_to_flags(mode);
    inode->super_block = &super_block;
    inode->access_time = inode->modify_time = inode->change_time = time_read_clock(CLOCK_REALTIME);
    inode->device = PROCFS_DEVICE_ID;
    inode->gid = gid;
    inode->uid = uid;
    inode->mode = mode;
    inode->i_op = S_ISDIR(mode) ? &procfs_dir_i_op : &procfs_i_op;
    inode->ref_count = 1;
    inode->parent = tparent;
    init_spinlock(&inode->lock);

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

static void procfs_refresh_directory(struct tnode *tnode) {
    struct procfs_data *data = tnode->inode->private_data;
    struct process *process = procfs_get_process(tnode->inode);

    procfs_directory_function_t refresher = PROCFS_GET_DIRECTORY_FUNCTION(data);

    refresher(tnode, process, PROCFS_IS_LOADED(data));
    PROCFS_MAKE_LOADED(data);
}

struct tnode *procfs_lookup(struct inode *inode, const char *name) {
    if (!(inode->flags & FS_DIR)) {
        struct procfs_buffer result = procfs_get_data(inode, false);
        inode->size = result.size;
        procfs_cleanup_data(result, inode);
    } else {
        procfs_refresh_directory(inode == root ? t_root : find_tnode_inode(inode->parent->inode->tnode_list, inode));
    }

    if (name == NULL) {
        return NULL;
    }

    struct tnode_list *list = inode->tnode_list;
    while (list != NULL) {
        if (strcmp(list->tnode->name, name) == 0) {
            return list->tnode;
        }
        list = list->next;
    }

    return NULL;
}

struct file *procfs_open(struct inode *inode, int flags, int *error) {
    assert(!(flags & O_RDWR));

    struct file *file = calloc(1, sizeof(struct file));
    file->device = inode->device;
    file->f_op = (inode->flags & FS_DIR) ? &procfs_dir_f_op : &procfs_f_op;
    file->flags = inode->flags;
    file->inode_idenifier = inode->index;
    file->length = inode->size;
    file->position = 0;
    file->start = 0;
    file->abilities = 0;
    file->ref_count = 0;

    *error = 0;
    return file;
}

int procfs_read_all(struct inode *inode, void *buffer) {
    struct procfs_buffer data = procfs_get_data(inode, true);

    memcpy(buffer, data.buffer, data.size);

    procfs_cleanup_data(data, inode);
    return 0;
}

ssize_t procfs_read(struct file *file, off_t offset, void *buffer, size_t len) {
    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    assert(inode);

    struct procfs_buffer data = procfs_get_data(inode, true);

    size_t to_read = MIN(len, data.size - offset);
    memcpy(buffer, data.buffer + offset, to_read);

    procfs_cleanup_data(data, inode);
    return to_read;
}

static struct procfs_buffer procfs_cwd(struct procfs_data *data __attribute__((unused)), struct process *process, bool need_buffer) {
    char *buffer = get_tnode_path(process->cwd);
    size_t length = strlen(buffer);
    if (!need_buffer) {
        free(buffer);
        buffer = NULL;
    }
    return (struct procfs_buffer) { buffer, length };
}

static struct procfs_buffer procfs_exe(struct procfs_data *data __attribute__((unused)), struct process *process, bool need_buffer) {
    char *buffer = get_tnode_path(process->exe);
    size_t length = strlen(buffer);
    if (!need_buffer) {
        free(buffer);
        buffer = NULL;
    }
    return (struct procfs_buffer) { buffer, length };
}

static struct procfs_buffer procfs_status(struct procfs_data *data __attribute__((unused)), struct process *process, bool need_buffer) {
    char *buffer = need_buffer ? aligned_alloc(PAGE_SIZE, PAGE_SIZE) : NULL;
    char tty_string[16];
    if (process->tty != -1) {
        snprintf(tty_string, sizeof(tty_string) - 1, "/dev/tty%d", process->tty);
    } else {
        snprintf(tty_string, sizeof(tty_string) - 1, "%s", "?");
    }

    struct task *main_task = find_by_tid(process->pgid, process->pid);
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
                 "TTY: %s\n",
                 process->name, process->pid, main_task ? task_state_to_string(main_task->sched_state) : "? (unknown)", process->uid,
                 process->gid, process->ppid, process->umask, process->euid, process->egid, process->pgid, process->sid, tty_string);
    return (struct procfs_buffer) { buffer, length };
}

static struct procfs_buffer procfs_vm(struct procfs_data *data __attribute__((unused)), struct process *process, bool need_buffer) {
    char *buffer = need_buffer ? aligned_alloc(PAGE_SIZE, PAGE_SIZE) : NULL;
    size_t length = 0;
    struct vm_region *vm = process->process_memory;
    while (vm) {
        length += snprintf(buffer + length, need_buffer ? PAGE_SIZE - length : 0,
                           "START: %p END: %p TYPE: %s\n"
                           "PERM: %c%c%c%c BACKED: %s\n",
                           (void *) vm->start, (void *) vm->end, vm_type_to_string(vm->type), (vm->flags & VM_PROT_NONE) ? ' ' : 'r',
                           (vm->flags & VM_WRITE) ? 'w' : ' ', (vm->flags & VM_NO_EXEC) ? ' ' : 'x', (vm->flags & VM_STACK) ? 's' : ' ',
                           vm->vm_object ? "yes" : "no");
        vm = vm->next;
    }

    return (struct procfs_buffer) { buffer, length };
}

static struct procfs_buffer procfs_signal(struct procfs_data *data __attribute__((unused)), struct process *process, bool need_buffer) {
    char *buffer = need_buffer ? aligned_alloc(PAGE_SIZE, PAGE_SIZE) : NULL;
    size_t length = 0;
    for (int i = 1; i < _NSIG; i++) {
        length += snprintf(
            buffer + length, need_buffer ? PAGE_SIZE - length : 0,
            "SIGNAL: %d (%s)\n"
            "STATE: %s\n"
            "FLAGS: %c%c%c%c%c%c%c\n"
            "MASK: %#.16lX\n",
            i, strsignal(i),
            process->sig_state[i].sa_handler == SIG_IGN ? "ignored" : process->sig_state[i].sa_handler == SIG_DFL ? "default" : "handled",
            process->sig_state[i].sa_flags & SA_NOCLDSTOP ? 'C' : ' ', process->sig_state[i].sa_flags & SA_ONSTACK ? 'S' : ' ',
            process->sig_state[i].sa_flags & SA_RESETHAND ? 'X' : ' ', process->sig_state[i].sa_flags & SA_RESTART ? 'R' : ' ',
            process->sig_state[i].sa_flags & SA_SIGINFO ? 'I' : ' ', process->sig_state[i].sa_flags & SA_NOCLDWAIT ? 'W' : ' ',
            process->sig_state[i].sa_flags & SA_NODEFER ? 'D' : ' ', process->sig_state[i].sa_mask);
    }
    return (struct procfs_buffer) { buffer, length };
}

static struct procfs_buffer procfs_fd(struct procfs_data *data, struct process *process, bool need_buffer) {
    int fd = data->fd;
    struct file *file = process->files[fd].file;
    if (!file) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    struct inode *inode = fs_inode_get(file->device, file->inode_idenifier);
    if (!inode) {
        return (struct procfs_buffer) { NULL, 0 };
    }

    struct tnode *fd_tnode = find_tnode_inode(inode->parent->inode->tnode_list, inode);

    char *buffer = get_tnode_path(fd_tnode);
    size_t length = strlen(buffer);
    if (!need_buffer) {
        free(buffer);
        buffer = NULL;
    }
    return (struct procfs_buffer) { buffer, length };
}

static void procfs_create_fd_directory_structure(struct tnode *tparent, struct process *process, bool loaded __attribute__((unused))) {
    struct inode *parent = tparent->inode;

    struct tnode_list *tnode = parent->tnode_list;
    while (tnode) {
        struct tnode_list *next = tnode->next;
        int fd = atoi(tnode->tnode->name);
        if (!process->files[fd].file) {
            struct tnode *real_tnode = tnode->tnode;
            parent->tnode_list = remove_tnode(parent->tnode_list, tnode->tnode);
            drop_tnode(real_tnode);
        }
        tnode = next;
    }

    struct tnode_list *node = tparent->inode->tnode_list;
    int node_fd = node ? atoi(node->tnode->name) : -1;
    for (int i = 0; i < FOPEN_MAX; i++) {
        struct file *file = process->files[i].file;
        if (file) {
            while (node && node_fd < i) {
                node = node->next;
                node_fd = atoi(node->tnode->name);
            }

            // Already in the tnode list
            if (node && node_fd == i) {
                continue;
            }

            char fd_string[32];
            snprintf(fd_string, sizeof(fd_string) - 1, "%d", i);

            struct inode *inode = procfs_create_inode(tparent, PROCFS_SYMLINK_MODE, process->uid, process->gid, process, procfs_fd);
            struct procfs_data *data = inode->private_data;
            data->fd = i;

            struct tnode *tnode = create_tnode(fd_string, inode);
            parent->tnode_list = add_tnode_before(parent->tnode_list, node, tnode);
        }
    }
}

static void procfs_create_process_directory_structure(struct tnode *tparent, struct process *process, bool loaded) {
    struct inode *parent = tparent->inode;

    if (!loaded) {
        struct inode *status_inode = procfs_create_inode(tparent, PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_status);
        struct tnode *status_tnode = create_tnode("status", status_inode);
        parent->tnode_list = add_tnode(parent->tnode_list, status_tnode);

        struct inode *cwd_inode = procfs_create_inode(tparent, PROCFS_SYMLINK_MODE, process->uid, process->gid, process, procfs_cwd);
        struct tnode *cwd_tnode = create_tnode("cwd", cwd_inode);
        parent->tnode_list = add_tnode(parent->tnode_list, cwd_tnode);

        struct inode *exe_inode = procfs_create_inode(tparent, PROCFS_SYMLINK_MODE, process->uid, process->gid, process, procfs_exe);
        struct tnode *exe_tnode = create_tnode("exe", exe_inode);
        parent->tnode_list = add_tnode(parent->tnode_list, exe_tnode);

        struct inode *vm_inode = procfs_create_inode(tparent, PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_vm);
        struct tnode *vm_tnode = create_tnode("vm", vm_inode);
        parent->tnode_list = add_tnode(parent->tnode_list, vm_tnode);

        struct inode *signal_inode = procfs_create_inode(tparent, PROCFS_FILE_MODE, process->uid, process->gid, process, procfs_signal);
        struct tnode *signal_tnode = create_tnode("signal", signal_inode);
        parent->tnode_list = add_tnode(parent->tnode_list, signal_tnode);

        struct inode *fd_inode =
            procfs_create_inode(tparent, PROCFS_DIRECTORY_MODE, process->uid, process->gid, process, procfs_create_fd_directory_structure);
        struct tnode *fd_tnode = create_tnode("fd", fd_inode);
        parent->tnode_list = add_tnode(parent->tnode_list, fd_tnode);
    }
}

static void procfs_destroy_process_directory_structure(struct inode *parent) {
    struct tnode_list *tnode_list_node = parent->tnode_list;
    while (tnode_list_node) {
        struct tnode *tnode = tnode_list_node->tnode;
        tnode_list_node = remove_tnode(tnode_list_node, tnode);
        // Recursively destroy all directory entries.
        if (tnode->inode->flags & FS_DIR) {
            procfs_destroy_process_directory_structure(tnode->inode);
        }
        drop_tnode(tnode);
    }
}

void procfs_register_process(struct process *process) {
    assert(root);

    struct inode *inode =
        procfs_create_inode(t_root, PROCFS_DIRECTORY_MODE, process->uid, process->gid, process, procfs_create_process_directory_structure);

    char pid_string[16];
    snprintf(pid_string, sizeof(pid_string) - 1, "%d", process->pid);
    struct tnode *parent = create_tnode(pid_string, inode);

    spin_lock(&root->lock);
    root->tnode_list = add_tnode(root->tnode_list, parent);
    spin_unlock(&root->lock);
}

void procfs_unregister_process(struct process *process) {
    assert(root);

    char pid_string[16];
    snprintf(pid_string, sizeof(pid_string) - 1, "%d", process->pid);
    struct tnode *tnode = find_tnode(root->tnode_list, pid_string);
    assert(tnode);

    spin_lock(&root->lock);
    root->tnode_list = remove_tnode(root->tnode_list, tnode);
    spin_unlock(&root->lock);

    procfs_destroy_process_directory_structure(tnode->inode);
    drop_tnode(tnode);
}

static struct procfs_buffer procfs_self(struct tnode *tnode __attribute__((unused)), struct process *process, bool need_buffer) {
    char *buffer = need_buffer ? aligned_alloc(PAGE_SIZE, PAGE_SIZE) : NULL;
    size_t length = snprintf(buffer, need_buffer ? PAGE_SIZE : 0, "%d", process->pid);
    return (struct procfs_buffer) { buffer, length };
}

static void procfs_create_base_directory_structure(struct tnode *tparent, struct process *process __attribute__((unused)), bool loaded) {
    assert(tparent);
    struct inode *parent = tparent->inode;

    if (!loaded) {
        struct inode *self_inode = procfs_create_inode(tparent, PROCFS_SYMLINK_MODE, 0, 0, NULL, procfs_self);
        struct procfs_data *data = self_inode->private_data;
        PROCFS_MAKE_DYNAMIC(data);

        struct tnode *self_tnode = create_tnode("self", self_inode);
        spin_lock(&parent->lock);
        parent->tnode_list = add_tnode(parent->tnode_list, self_tnode);
        spin_unlock(&parent->lock);
    }
}

struct tnode *procfs_mount(struct file_system *current_fs, char *device_path) {
    assert(current_fs != NULL);
    assert(strlen(device_path) == 0);

    root = procfs_create_inode(NULL, PROCFS_DIRECTORY_MODE, 0, 0, NULL, procfs_create_base_directory_structure);
    struct procfs_data *root_data = root->private_data;
    PROCFS_MAKE_DYNAMIC(root_data);

    t_root = create_root_tnode(root);

    super_block.device = root->device;
    super_block.op = NULL;
    super_block.root = t_root;
    super_block.block_size = PAGE_SIZE;

    current_fs->super_block = &super_block;
    return t_root;
}

void init_procfs() {
    load_fs(&fs);
}