#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/os_2.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <kernel/fs/file.h>
#include <kernel/fs/vfs.h>
#include <kernel/mem/anon_vm_object.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/socket.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/validators.h>

#include <kernel/arch/x86_64/proc/task.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/hal/x86_64/gdt.h>
#include <kernel/irqs/handlers.h>

// #define DUP_DEBUG
// #define SET_PGID_DEBUG
// #define SIGACTION_DEBUG
// #define SIGPROCMASK_DEBUG
// #define SIGRETURN_DEBUG
// #define SYSCALL_DEBUG
// #define USER_MUTEX_DEBUG
// #define WAIT_PID_DEBUG

#define SYS_CALL(n) void arch_sys_##n(struct task_state *task_state)

#define SYS_VALIDATE(n, a, f)               \
    do {                                    \
        __DO_VALIDATE(n, a, f, SYS_RETURN); \
    } while (0)

#define SYS_PARAM(t, n, r) t n = (t) task_state->cpu_state.r
#define SYS_PARAM_VALIDATE(t, n, r, f, a) \
    SYS_PARAM(t, n, r);                   \
    do {                                  \
        SYS_VALIDATE(n, a, f);            \
    } while (0)
#define SYS_PARAM_TRANSFORM(t, n, ot, r, f)            \
    t n;                                               \
    do {                                               \
        int ret = f((ot) task_state->cpu_state.r, &n); \
        if (ret < 0) {                                 \
            SYS_RETURN(ret);                           \
        }                                              \
    } while (0)

#define SYS_PARAM1(t, n) SYS_PARAM(t, n, rsi)
#define SYS_PARAM2(t, n) SYS_PARAM(t, n, rdx)
#define SYS_PARAM3(t, n) SYS_PARAM(t, n, rcx)
#define SYS_PARAM4(t, n) SYS_PARAM(t, n, r8)
#define SYS_PARAM5(t, n) SYS_PARAM(t, n, r9)
#define SYS_PARAM6(t, n) SYS_PARAM(t, n, r10)

#define SYS_PARAM1_VALIDATE(t, n, f, a) SYS_PARAM_VALIDATE(t, n, rsi, f, a)
#define SYS_PARAM2_VALIDATE(t, n, f, a) SYS_PARAM_VALIDATE(t, n, rdx, f, a)
#define SYS_PARAM3_VALIDATE(t, n, f, a) SYS_PARAM_VALIDATE(t, n, rcx, f, a)
#define SYS_PARAM4_VALIDATE(t, n, f, a) SYS_PARAM_VALIDATE(t, n, r8, f, a)
#define SYS_PARAM5_VALIDATE(t, n, f, a) SYS_PARAM_VALIDATE(t, n, r9, f, a)
#define SYS_PARAM6_VALIDATE(t, n, f, a) SYS_PARAM_VALIDATE(t, n, r10, f, a)

#define SYS_PARAM1_TRANSFORM(t, n, ot, f) SYS_PARAM_TRANSFORM(t, n, ot, rsi, f)
#define SYS_PARAM2_TRANSFORM(t, n, ot, f) SYS_PARAM_TRANSFORM(t, n, ot, rdx, f)
#define SYS_PARAM3_TRANSFORM(t, n, ot, f) SYS_PARAM_TRANSFORM(t, n, ot, rcx, f)
#define SYS_PARAM4_TRANSFORM(t, n, ot, f) SYS_PARAM_TRANSFORM(t, n, ot, r8, f)
#define SYS_PARAM5_TRANSFORM(t, n, ot, f) SYS_PARAM_TRANSFORM(t, n, ot, r9, f)
#define SYS_PARAM6_TRANSFORM(t, n, ot, f) SYS_PARAM_TRANSFORM(t, n, ot, r10, f)

#define SYS_BEGIN()                                                            \
    do {                                                                       \
        get_current_task()->arch_task.user_task_state = (task_state);          \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = -EINTR; \
        get_current_task()->in_kernel = true;                                  \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;             \
        enable_interrupts();                                                   \
    } while (0)

#define SYS_BEGIN_CAN_SEND_SELF_SIGNALS()                                 \
    do {                                                                  \
        get_current_task()->arch_task.user_task_state = (task_state);     \
        get_current_task()->in_kernel = true;                             \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;        \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = 0; \
    } while (0)

#define SYS_BEGIN_SIGSUSPEND()                                                 \
    do {                                                                       \
        get_current_task()->arch_task.user_task_state = (task_state);          \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = -EINTR; \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;             \
        get_current_task()->in_kernel = true;                                  \
        get_current_task()->in_sigsuspend = true;                              \
    } while (0)

#define SYS_BEGIN_PSELECT()                                                   \
    do {                                                                      \
        get_current_task()->arch_task.user_task_state = (task_state);         \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = EINTR; \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;            \
        get_current_task()->in_kernel = true;                                 \
    } while (0)

#define SYS_RETURN(val)                                       \
    do {                                                      \
        uint64_t _val = (uint64_t)(val);                      \
        disable_interrupts();                                 \
        task_state->cpu_state.rax = (_val);                   \
        task_do_sigs_if_needed(get_current_task());           \
        get_current_task()->in_kernel = false;                \
        get_current_task()->arch_task.user_task_state = NULL; \
        return;                                               \
    } while (0)

#define SYS_RETURN_DONT_CHECK_SIGNALS(val)                    \
    do {                                                      \
        uint64_t _val = (uint64_t)(val);                      \
        disable_interrupts();                                 \
        task_state->cpu_state.rax = (_val);                   \
        get_current_task()->in_kernel = false;                \
        get_current_task()->arch_task.user_task_state = NULL; \
        return;                                               \
    } while (0)

#define SYS_RETURN_RESTORE_SIGMASK(val)                                         \
    do {                                                                        \
        uint64_t _val = (uint64_t)(val);                                        \
        disable_interrupts();                                                   \
        task_state->cpu_state.rax = (_val);                                     \
        task_do_sigs_if_needed(get_current_task());                             \
        memcpy(&current->sig_mask, &current->saved_sig_mask, sizeof(sigset_t)); \
        get_current_task()->in_kernel = false;                                  \
        get_current_task()->in_sigsuspend = false;                              \
        return;                                                                 \
    } while (0)

extern struct task *current_task;

static int get_file(int fd, struct file **file) {
    if (fd < 0 || fd >= FOPEN_MAX) {
        return -EBADF;
    }

    struct process *current = get_current_task()->process;
    if (!current->files[fd].file) {
        return -EBADF;
    }

    *file = current->files[fd].file;
    return 0;
}

static int get_file_desc(int fd, struct file_descriptor **desc) {
    if (fd < 0 || fd >= FOPEN_MAX) {
        return -EBADF;
    }

    struct process *current = get_current_task()->process;
    if (!current->files[fd].file) {
        return -EBADF;
    }

    *desc = &current->files[fd];
    return 0;
}

static int get_at_directory(int fd, struct tnode **tnode) {
    struct process *current = get_current_task()->process;
    if (fd == AT_FDCWD) {
        *tnode = current->cwd;
        return 0;
    }

    if (fd < 0 || fd >= FOPEN_MAX) {
        return -EBADF;
    }

    struct file *file = current->files[fd].file;
    if (!file) {
        return -EBADF;
    }

    if (!(file->flags & FS_DIR)) {
        return -ENOTDIR;
    }

    *tnode = fs_get_tnode_for_file(file);
    if (!*tnode) {
        return -ENOTDIR;
    }
    return 0;
}

static int get_socket(int fd, struct file **filep) {
    if (fd < 0 || fd > FOPEN_MAX) {
        return -EBADF;
    }

    struct file *file = get_current_task()->process->files[fd].file;
    if (!file) {
        return -EBADF;
    }

    if (!(file->flags & FS_SOCKET)) {
        return -ENOTSOCK;
    }

    *filep = file;
    return 0;
}

SYS_CALL(exit) {
    SYS_BEGIN();

    /* Disable Interrups To Prevent Premature Task Removal, Since Sched State Is Set */
    disable_interrupts();

    struct task *task = get_current_task();
    exit_process(task->process);

    invalidate_last_saved(task);

    int exit_code = (int) task_state->cpu_state.rsi;
    proc_add_message(task->process->pid, proc_create_message(STATE_EXITED, exit_code));
    debug_log("Process Exited: [ %d, %d ]\n", task->process->pid, exit_code);

    sys_sched_run_next(task_state);
}

SYS_CALL(sbrk) {
    SYS_BEGIN();

    SYS_PARAM1(intptr_t, increment);

    void *res;
    if (increment < 0) {
        res = add_vm_pages_end(0, VM_PROCESS_HEAP);
        remove_vm_pages_end(-increment, VM_PROCESS_HEAP);
    } else {
        res = add_vm_pages_end(increment, VM_PROCESS_HEAP);
    }

    if (res == NULL) {
        SYS_RETURN(-ENOMEM);
    }

    SYS_RETURN(res);
}

SYS_CALL(fork) {
    SYS_BEGIN();

    struct task *parent = get_current_task();
    struct task *child = calloc(1, sizeof(struct task));
    struct process *child_process = calloc(1, sizeof(struct process));
    child->process = child_process;

    child->tid = get_next_tid();
    child_process->pid = get_next_pid();
    init_spinlock(&child_process->lock);
    proc_add_process(child_process);
    child->sched_state = RUNNING_INTERRUPTIBLE;
    child->kernel_task = false;
    child_process->process_memory = clone_process_vm();
    child_process->tty = parent->process->tty;

    debug_log("Forking Task: [ %d ]\n", parent->process->pid);

    memcpy(&child->arch_task.task_state, task_state, sizeof(struct task_state));
    child->arch_task.task_state.cpu_state.rax = 0;
    child_process->arch_process.cr3 = clone_process_paging_structure();
    child->arch_task.kernel_stack = KERNEL_TASK_STACK_START;
    child->arch_task.setup_kernel_stack = true;
    child->arch_task.user_thread_pointer = parent->arch_task.user_thread_pointer;
    child->process->tls_master_copy_start = parent->process->tls_master_copy_start;
    child->process->tls_master_copy_size = parent->process->tls_master_copy_size;
    child->process->tls_master_copy_alignment = parent->process->tls_master_copy_alignment;
    child_process->cwd = bump_tnode(parent->process->cwd);
    child_process->pgid = parent->process->pgid;
    child_process->ppid = parent->process->pid;
    child->process->uid = parent->process->uid;
    child->process->euid = parent->process->euid;
    child->process->gid = parent->process->gid;
    child->process->egid = parent->process->egid;
    child->process->sid = parent->process->sid;
    child->process->umask = parent->process->umask;
    child->sig_pending = 0;
    child->sig_mask = parent->sig_mask;
    child_process->inode_dev = parent->process->inode_dev;
    child_process->inode_id = parent->process->inode_id;
    memcpy(&child_process->sig_state, &parent->process->sig_state, sizeof(struct sigaction) * _NSIG);

    task_align_fpu(child);

    // Clone fpu if necessary
    if (parent->fpu.saved) {
        child->fpu.saved = true;
        memcpy(child->fpu.aligned_state, parent->fpu.aligned_state, sizeof(child->fpu.raw_fpu_state.image));
    }

    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (parent->process->files[i].file) {
            child_process->files[i] = fs_dup(parent->process->files[i]);
        }
    }

    disable_interrupts();
    sched_add_task(child);
    SYS_RETURN(child_process->pid);
}

SYS_CALL(openat) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct tnode *, base, int, get_at_directory);
    SYS_PARAM2_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM3(int, flags);
    SYS_PARAM4(mode_t, mode);

    assert(path != NULL);

    int error = 0;

    struct task *task = get_current_task();

    struct file *file = fs_openat(base, path, flags, &error);

    if (file && (flags & O_EXCL)) {
        fs_close(file);
        SYS_RETURN(-EEXIST);
    }

    if (file == NULL) {
        if (flags & O_CREAT) {
            debug_log("Creating file: [ %s ]\n", path);

            error = fs_create(path, mode | S_IFREG);
            if (error) {
                SYS_RETURN((uint64_t) error);
            }

            file = fs_open(path, flags, &error);
            if (file == NULL) {
                SYS_RETURN((uint64_t) error);
            }

            debug_log("Open successful\n");
        } else {
            debug_log("File Not Found: [ %s ]\n", path);
            SYS_RETURN((uint64_t) error);
        }
    }

    /* Should probably be some other error instead */
    if (!(file->flags & FS_DIR) && (flags & O_DIRECTORY)) {
        SYS_RETURN(-EINVAL);
    }

    if (file->flags & FS_DIR && !(flags & O_DIRECTORY)) {
        SYS_RETURN(-EISDIR);
    }

    /* Handle append mode */
    if (flags & O_APPEND) {
        fs_seek(file, 0, SEEK_END);
    }

    for (int i = 0; i < FOPEN_MAX; i++) {
        if (task->process->files[i].file == NULL) {
            task->process->files[i].file = file;
            task->process->files[i].fd_flags = (flags & O_CLOEXEC) ? FD_CLOEXEC : 0;
            SYS_RETURN(i);
        }
    }

    SYS_RETURN(-EMFILE);
}

SYS_CALL(read) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM3(size_t, count);
    SYS_PARAM2_VALIDATE(char *, buf, validate_write, count);

    SYS_RETURN(fs_read(file, buf, count));
}

SYS_CALL(write) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM3(size_t, count);
    SYS_PARAM2_VALIDATE(const void *, buf, validate_read, count);

    SYS_RETURN(fs_write(file, buf, count));
}

SYS_CALL(close) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file_descriptor *, desc, int, get_file_desc);

    int error = fs_close(desc->file);
    desc->file = NULL;

    SYS_RETURN(error);
}

static int execve_helper(const char **path, char **buffer, size_t *buffer_length, char ***prepend_argv, size_t *prepend_argv_length,
                         dev_t *device, ino_t *ino_id, char **argv) {
    struct inode *inode;
    int ret = fs_read_all_path(*path, (void **) buffer, buffer_length, &inode);
    if (ret < 0) {
        return ret;
    }

    if (*buffer_length == 0) {
        return -ENOEXEC;
    }

    if (!elf64_is_valid(*buffer)) {
        if (memcmp(*buffer, "#!", 2) == 0) {
            debug_log("Encoutered #!\n");
            bool first = *prepend_argv_length == 0;

            const char *path_save = NULL;
            if (first) {
                path_save = *path;
            }
            size_t path_len = strcspn(*buffer + 2, " \n");
            char restore = (*buffer)[2 + path_len];
            (*buffer)[2 + path_len] = '\0';
            *path = *buffer + 2;
            debug_log("#!: [ %s ]\n", *path);
            (*buffer)[2 + path_len] = restore;
            bool has_extra_arg = false;
            size_t extra_arg_start = 0;

            size_t i;
            for (i = 0; (*buffer)[2 + path_len + i] != '\n' && (*buffer)[2 + path_len + i] != '\0'; i++) {
                if (!extra_arg_start && !isspace((*buffer)[2 + path_len + i])) {
                    has_extra_arg = true;
                    extra_arg_start = 2 + path_len + i;
                    break;
                }
            }

            (*prepend_argv_length) += first;

            if (has_extra_arg) {
                (*prepend_argv_length) += 2;
            } else {
                (*prepend_argv_length)++;
            }

            *prepend_argv = realloc(*prepend_argv, *prepend_argv_length * sizeof(char *));
            if (has_extra_arg) {
                *buffer[2 + path_len] = '\0';
                *prepend_argv[*prepend_argv_length - 3] = strdup(*buffer + 2);
                *buffer[2 + path_len] = restore;
                *buffer[2 + path_len + i] = '\0';
                *prepend_argv[*prepend_argv_length - 2] = strdup(*buffer + extra_arg_start);
            } else {
                *buffer[2 + path_len] = '\0';
                *prepend_argv[*prepend_argv_length - 2] = strdup(*buffer + 2);
                *buffer[2 + path_len] = restore;
            }

            (*prepend_argv)[*prepend_argv_length - 1] = NULL;

            if (first) {
                argv[0] = (char *) path_save;
            }

            free(*buffer);
            *buffer = NULL;
            return execve_helper(path, buffer, buffer_length, prepend_argv, prepend_argv_length, device, ino_id, argv);
        }

        return -ENOEXEC;
    }

    *device = inode->device;
    *ino_id = inode->index;

    return 0;
}

SYS_CALL(execve) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM2_VALIDATE(char **, argv, validate_string_array, -1);
    SYS_PARAM3_VALIDATE(char **, envp, validate_string_array, -1);

    assert(path != NULL);
    assert(argv != NULL);
    assert(envp != NULL);

    struct task *current = get_current_task();

    debug_log("Exec Task: [ %d, %s ]\n", current->process->pid, path);

    char *buffer = NULL;
    size_t length = 0;
    char **prepend_argv = NULL;
    size_t prepend_argv_length = 0;
    dev_t device = 0;
    ino_t ino_id = 0;
    int error = execve_helper(&path, &buffer, &length, &prepend_argv, &prepend_argv_length, &device, &ino_id, argv);
    if (error) {
        for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
            free(prepend_argv[i]);
        }
        free(prepend_argv);

        free(buffer);
        SYS_RETURN(error);
    }

    struct task *task = calloc(1, sizeof(struct task));
    struct process *process = calloc(1, sizeof(struct process));
    task->process = process;
    process->inode_dev = device;
    process->inode_id = ino_id;

    // Dup open file descriptors
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (!current->process->files[i].file ||
            ((current->process->files[i].fd_flags & FD_CLOEXEC) || (current->process->files[i].file->flags & FS_DIR))) {
            // NOTE: the files will be closed by the `free_task` function
            continue;
        }

        process->files[i] = fs_dup(current->process->files[i]);
    }

    /* Clone vm_regions so that they can be freed later */
    struct vm_region *__process_stack = get_vm_last_region(current->process->process_memory, VM_TASK_STACK);
    struct vm_region *__process_guard = get_vm_last_region(current->process->process_memory, VM_TASK_STACK_GUARD);
    assert(__process_stack);
    assert(__process_guard);

    struct vm_region *__kernel_stack = get_vm_region(current->process->process_memory, VM_KERNEL_STACK);

    struct vm_region *process_stack = calloc(1, sizeof(struct vm_region));
    struct vm_region *process_guard = calloc(1, sizeof(struct vm_region));
    struct vm_region *kernel_stack = calloc(1, sizeof(struct vm_region));

    memcpy(process_stack, __process_stack, sizeof(struct vm_region));
    memcpy(process_guard, __process_guard, sizeof(struct vm_region));
    memcpy(kernel_stack, __kernel_stack, sizeof(struct vm_region));

    task->tid = current->tid;
    process->pid = current->process->pid;
    init_spinlock(&process->lock);
    process->pgid = current->process->pgid;
    process->ppid = current->process->ppid;
    process->uid = current->process->uid;
    process->euid = current->process->euid;
    process->gid = current->process->gid;
    process->egid = current->process->egid;
    process->sid = current->process->sid;
    process->umask = current->process->umask;
    process->process_memory = kernel_stack;
    process->process_memory = add_vm_region(process->process_memory, process_stack);
    process->process_memory = add_vm_region(process->process_memory, process_guard);
    task->kernel_task = false;
    task->sched_state = RUNNING_INTERRUPTIBLE;
    process->tty = current->process->tty;
    process->cwd = bump_tnode(current->process->cwd);
    task->next = NULL;
    task->sig_mask = current->sig_mask;
    memcpy(&process->times, &current->process->times, sizeof(struct tms));

    // Clone only signal dispositions that don't have a handler
    for (int i = 0; i < _NSIG; i++) {
        if ((uintptr_t) current->process->sig_state[i].sa_handler <= (uintptr_t) SIG_IGN) {
            memcpy(&process->sig_state[i], &current->process->sig_state[i], sizeof(struct sigaction));
        }
    }

    process->arch_process.cr3 = get_cr3();
    task->arch_task.kernel_stack = KERNEL_TASK_STACK_START;

    struct virt_page_info *info = calloc(1, sizeof(struct virt_page_info));
    memcpy(info, current->arch_task.kernel_stack_info, sizeof(struct virt_page_info));

    task->arch_task.kernel_stack_info = info;
    task->arch_task.setup_kernel_stack = false;

    task_align_fpu(task);

    task->arch_task.task_state.cpu_state.rbp = KERNEL_TASK_STACK_START;
    task->arch_task.task_state.stack_state.rip = elf64_get_entry(buffer);
    task->arch_task.task_state.stack_state.cs = USER_CODE_SELECTOR;
    task->arch_task.task_state.stack_state.rflags = get_rflags() | INTERRUPS_ENABLED_FLAG;
    task->arch_task.task_state.stack_state.rsp = map_program_args(process_stack->end, prepend_argv, argv, envp);
    task->arch_task.task_state.stack_state.ss = USER_DATA_SELECTOR;

    for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
        free(prepend_argv[i]);
    }
    free(prepend_argv);

    /* Memset stack to zero so that task can use old one safely (only go until rsp because args are after it). */
    // FIXME: this will forcibily load the entire stack into memory. Instead we should map in a completely new
    // stack
    memset((void *) process_stack->start, 0, task->arch_task.task_state.stack_state.rsp - process_stack->start);

    /* Ensure File Name And Args Are Still Mapped */
    soft_remove_paging_structure(current->process->process_memory);

    elf64_load_program(buffer, length, task);
    elf64_map_heap(buffer, task);

    free(buffer);

    /* Disable Preemption So That Nothing Goes Wrong When Removing Ourselves (We Don't Want To Remove Ourselves From The List And Then Be
     * Interrupted) */
    disable_interrupts();

    // NOTE: this is necessary b/c current_task must always be valid. Otherwise, we will save state
    //       automatically on a pointer to a freed address, corrupting memory.
    current_task = task;

    sched_remove_task(current);
    invalidate_last_saved(current);
    free_task(current, false);
    assert(get_current_task() == task);
    sched_add_task(task);
    proc_add_process(process);

    sys_sched_run_next(&task->arch_task.task_state);
}

SYS_CALL(waitpid) {
    SYS_BEGIN();

    SYS_PARAM1(pid_t, pid);
    SYS_PARAM2_VALIDATE(int *, status, validate_write, sizeof(int));
    SYS_PARAM3(int, flags);

    struct task *current = get_current_task();

    if (pid == 0) {
        pid = -current->process->pgid;
    }

#ifdef WAIT_PID_DEBUG
    debug_log("Waiting on pid: [ %d ]\n", pid);
#endif /* WAIT_PID_DEBUG */

    struct proc_state_message m;
    pid_t found_pid;
    for (;;) {
        if (pid < -1) {
            found_pid = proc_consume_message_by_pg(-pid, &m);
        } else if (pid == -1) {
            found_pid = proc_consume_message_by_parent(current->process->pid, &m);
        } else {
            found_pid = proc_consume_message(pid, &m);
        }

        if (found_pid == 0) {
            if (flags & WNOHANG) {
#ifdef WAIT_PID_DEBUG
                debug_log("Wait found nothing: [ %d ]\n", pid);
#endif /* WAIT_PID_DEBUG */
                SYS_RETURN(-ECHILD);
            } else {
                proc_block_waitpid(current, pid);
            }
        } else {
            if ((m.type == STATE_STOPPED && !(flags & WUNTRACED)) || (m.type == STATE_CONTINUED && !(flags & WCONTINUED))) {
                continue;
            }
            break;
        }
    }

#ifdef WAIT_PID_DEBUG
    debug_log("Waited out pid: [ %d, %d ]\n", found_pid, m.type);
#endif /* WAIT_PID_DEBUG */

    if (!status) {
        SYS_RETURN(found_pid);
    }

    if (m.type == STATE_EXITED) {
        *status = (m.data & 0xFF) << 8;
    } else if (m.type == STATE_STOPPED) {
        *status = 0x80 | ((m.data & 0xFF) << 8);
    } else if (m.type == STATE_INTERRUPTED) {
        *status = m.data;
    } else if (m.type == STATE_CONTINUED) {
        *status = 0xFFFF;
    } else {
        assert(false);
    }

    // Indicated Success
    SYS_RETURN(found_pid);
}

SYS_CALL(getpid) {
    SYS_BEGIN();
    SYS_RETURN(get_current_task()->process->pid);
}

SYS_CALL(getcwd) {
    SYS_BEGIN();

    SYS_PARAM2(size_t, size);
    SYS_PARAM1_VALIDATE(char *, buffer, validate_write, size);

    struct task *current = get_current_task();
    char *full_path = get_tnode_path(current->process->cwd);

    size_t len = strlen(full_path);
    if (len > size) {
        free(full_path);
        SYS_RETURN(-ERANGE);
    }

    strcpy(buffer, full_path);

    free(full_path);
    SYS_RETURN(buffer);
}

SYS_CALL(chdir) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);

    struct task *task = get_current_task();

    struct tnode *tnode;
    int ret = iname(path, 0, &tnode);
    if (ret < 0) {
        SYS_RETURN(ret);
    }

    if (!(tnode->inode->flags & FS_DIR)) {
        SYS_RETURN(-ENOTDIR);
    }

    task->process->cwd = bump_tnode(tnode);

    SYS_RETURN(0);
}

SYS_CALL(stat) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM2_VALIDATE(void *, stat_struct, validate_write, sizeof(struct stat));

    SYS_RETURN(fs_stat(path, stat_struct));
}

SYS_CALL(lseek) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM2(off_t, offset);
    SYS_PARAM3(int, whence);

    SYS_RETURN(fs_seek(file, offset, whence));
}

SYS_CALL(ioctl) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM2(unsigned long, request);
    SYS_PARAM3(void *, argp);

    SYS_RETURN(fs_ioctl(file, request, argp));
}

SYS_CALL(ftruncate) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM2(off_t, length);

    SYS_RETURN(fs_truncate(file, length));
}

SYS_CALL(gettimeofday) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(struct timeval *, tv, validate_write_or_null, sizeof(struct timeval));
    SYS_PARAM2_VALIDATE(struct timezone *, tz, validate_write_or_null, sizeof(struct timezone));

    time_t micro_seconds = get_time();
    if (tv) {
        tv->tv_sec = micro_seconds / 1000;
        tv->tv_usec = (micro_seconds % 1000) * 1000;
    }

    if (tz) {
        tz->tz_minuteswest = 0;
        tz->tz_minuteswest = 0;
    }

    SYS_RETURN(0);
}

SYS_CALL(mkdir) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, pathname, validate_path, -1);
    SYS_PARAM2(mode_t, mode);

    SYS_RETURN(fs_mkdir(pathname, mode));
}

SYS_CALL(dup2) {
    SYS_BEGIN();

    // FIXME: validate oldfd?
    SYS_PARAM1(int, oldfd);
    SYS_PARAM2(int, newfd);

#ifdef DUP_DEBUG
    debug_log("Dup: [ %d, %d ]\n", oldfd, newfd);
#endif /* DUP_DEBUG */

    if (oldfd < 0 || oldfd >= FOPEN_MAX || newfd < 0 || newfd >= FOPEN_MAX) {
        SYS_RETURN((uint64_t) -EBADFD);
    }

    struct task *task = get_current_task();
    if (task->process->files[newfd].file != NULL) {
        int ret = fs_close(task->process->files[newfd].file);
        if (ret != 0) {
            SYS_RETURN((uint64_t) ret);
        }
    }

    task->process->files[newfd] = fs_dup(task->process->files[oldfd]);
    SYS_RETURN(0);
}

SYS_CALL(pipe) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(int *, pipefd, validate_write, 2 * sizeof(int));

    struct file *pipe_files[2];
    int ret = fs_create_pipe(pipe_files);
    if (ret != 0) {
        SYS_RETURN(ret);
    }

    struct task *task = get_current_task();
    int j = 0;
    for (int i = 0; j < 2 && i < FOPEN_MAX; i++) {
        if (task->process->files[i].file == NULL) {
            debug_log("Allocating pipe to: [ %d, %d ]\n", i, j);
            task->process->files[i] = (struct file_descriptor) { pipe_files[j], 0 };
            pipefd[j] = i;
            j++;
        }
    }

    assert(j == 2);

    SYS_RETURN(0);
}

SYS_CALL(unlink) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);

    SYS_RETURN(fs_unlink(path));
}

SYS_CALL(rmdir) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);

    SYS_RETURN(fs_rmdir(path));
}

SYS_CALL(chmod) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM2(mode_t, mode);

    SYS_RETURN(fs_chmod(path, mode));
}

SYS_CALL(kill) {
    SYS_BEGIN_CAN_SEND_SELF_SIGNALS();

    SYS_PARAM1(pid_t, pid);
    SYS_PARAM2_VALIDATE(int, signum, validate_signal_number, -1);

    struct task *current = get_current_task();

    // pid -1 is not yet implemented
    if (pid == -1) {
        SYS_RETURN(-EINVAL);
    }

    if (pid == 0) {
        pid = -current->process->pgid;
    }

    if (pid < 0) {
        SYS_RETURN(signal_process_group(-pid, signum));
    } else {
        SYS_RETURN(signal_process(pid, signum));
    }
}

SYS_CALL(setpgid) {
    SYS_BEGIN();

    SYS_PARAM1(pid_t, pid);
    SYS_PARAM2(pid_t, pgid);

    if (pgid < 0) {
        SYS_RETURN(-EINVAL);
    }

    if (pid == 0) {
        pid = get_current_task()->process->pid;
    }

    if (pgid == 0) {
        pgid = get_current_task()->process->pid;
    }

#ifdef SET_PGID_DEBUG
    debug_log("Setting pgid: [ %d, %d, %d ]\n", pid, get_current_task()->pgid, pgid);
#endif /* SET_PGID_DEBUG */

    struct process *process = find_by_pid(pid);
    if (process == NULL) {
        SYS_RETURN(-ESRCH);
    }

    spin_lock(&process->lock);
    process->pgid = pgid;
    proc_update_pgid(pid, pgid);
    spin_unlock(&process->lock);

    SYS_RETURN(0);
}

SYS_CALL(sigaction) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(int, signum, validate_signal_number, -1);
    SYS_PARAM2_VALIDATE(const struct sigaction *, act, validate_read_or_null, sizeof(struct sigaction));
    SYS_PARAM3_VALIDATE(struct sigaction *, old_act, validate_write_or_null, sizeof(struct sigaction));

    if (signum <= 0 || signum > _NSIG) {
        SYS_RETURN(-EINVAL);
    }

    struct task *current = get_current_task();
    if (old_act != NULL) {
        memcpy(old_act, &current->process->sig_state[signum], sizeof(struct sigaction));
    }

    if (act != NULL) {
#ifdef SIGACTION_DEBUG
        debug_log("Changing signal state: [ %d, %#.16lX ]\n", signum, (uintptr_t) act->sa_handler);
#endif /* SIGACTION_DEBUG */
        memcpy(&current->process->sig_state[signum], act, sizeof(struct sigaction));
        if (signum == SIGCHLD && act->sa_handler == SIG_DFL) {
            current->process->sig_state[signum].sa_flags |= SA_NOCLDWAIT;
        }
    }

    SYS_RETURN(0);
}

SYS_CALL(sigreturn) {
    struct task *task = get_current_task();
    siginfo_t *info = (siginfo_t *) (((uint64_t *) task_state->stack_state.rsp) + 1);
    ucontext_t *context = (ucontext_t *) (info + 1);
    uint8_t *saved_fpu_state = (uint8_t *) (context + 1);
    debug_log("Sig return: [ %p ]\n", context);

    memcpy(task->fpu.aligned_state, saved_fpu_state, FPU_IMAGE_SIZE);
    memcpy(&task->arch_task.task_state, &context->uc_mcontext, sizeof(struct task_state));

    // Restore mask
    task->sig_mask = context->uc_sigmask;

#ifdef SIGRETURN_DEBUG
    debug_log("State: [ %#.16lX, %#.16lX, %#.16lX, %#.16lX, %#.16lX ]\n", task->arch_task.task_state.stack_state.cs,
              task->arch_task.task_state.stack_state.rip, task->arch_task.task_state.stack_state.rflags,
              task->arch_task.task_state.stack_state.rsp, task->arch_task.task_state.stack_state.ss);
#endif /* SIGRETURN_DEBUG */

    __run_task(&task->arch_task);
}

SYS_CALL(sigprocmask) {
    SYS_BEGIN();

    SYS_PARAM1(int, how);
    SYS_PARAM2_VALIDATE(const sigset_t *, set, validate_read_or_null, sizeof(sigset_t));
    SYS_PARAM3_VALIDATE(sigset_t *, old, validate_write_or_null, sizeof(sigset_t));

    struct task *current = get_current_task();

    if (old) {
        *old = current->sig_mask;
    }

    if (set) {
#ifdef SIGPROCMASK_DEBUG
        debug_log("Setting sigprocmask: [ %d, %u ]\n", how, *set);
#endif /* SIGPROCMASK_DEBUG */

        switch (how) {
            case SIG_SETMASK:
                current->sig_mask = *set;
                break;
            case SIG_BLOCK:
                current->sig_mask |= *set;
                break;
            case SIG_UNBLOCK:
                current->sig_mask &= ~*set;
                break;
            default:
                SYS_RETURN(-EINVAL);
        }

#ifdef SIGPROCMASK_DEBUG
        debug_log("New mask: [ %u ]\n", current->sig_mask);
#endif /* SIGPROCMASK_DEBUG */
    }

    SYS_RETURN(0);
}

SYS_CALL(dup) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file_descriptor *, old_desc, int, get_file_desc);

    struct task *current = get_current_task();

    // Should lock task to prevent races
    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (current->process->files[i].file == NULL) {
            current->process->files[i] = fs_dup(*old_desc);
#ifdef DUP_DEBUG
            debug_log("Dup: [ %d, %lu ]\n", oldfd, i);
#endif /* DUP_DEBUG */
            SYS_RETURN(i);
        }
    }

    SYS_RETURN(-EMFILE);
}

SYS_CALL(getpgid) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(pid_t, pid, validate_positive, 1);
    if (pid == 0) {
        pid = get_current_task()->process->pid;
    }

    SYS_RETURN(proc_get_pgid(pid));
}

SYS_CALL(sleep) {
    SYS_BEGIN();

    SYS_PARAM1(unsigned int, seconds);

    debug_log("Sleeping: [ %u ]\n", seconds);

    struct task *current = get_current_task();

    time_t end_time = get_time() + seconds * 1000;

    proc_block_sleep_milliseconds(current, end_time);

    time_t now = get_time();
    SYS_RETURN(now < end_time ? end_time - now : 0);
}

SYS_CALL(access) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM2(int, mode);

    SYS_RETURN(fs_access(path, mode));
}

SYS_CALL(accept4) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3_VALIDATE(socklen_t *, addrlen, validate_write, sizeof(socklen_t));
    SYS_PARAM2_VALIDATE(struct sockaddr *, addr, validate_write, *addrlen);
    SYS_PARAM4(int, flags);

    SYS_RETURN(net_accept(file, addr, addrlen, flags));
}

SYS_CALL(bind) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3(socklen_t, addrlen);
    SYS_PARAM2_VALIDATE(const struct sockaddr *, addr, validate_read, addrlen);

    SYS_RETURN(net_bind(file, addr, addrlen));
}

SYS_CALL(connect) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3(socklen_t, addrlen);
    SYS_PARAM2_VALIDATE(const struct sockaddr *, addr, validate_read, addrlen);

    SYS_RETURN(net_connect(file, addr, addrlen));
}

SYS_CALL(listen) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM2_VALIDATE(int, backlog, validate_positive, 0);

    SYS_RETURN(net_listen(file, backlog));
}

SYS_CALL(socket) {
    SYS_BEGIN();

    SYS_PARAM1(int, domain);
    SYS_PARAM2(int, type);
    SYS_PARAM3(int, protocol);

    SYS_RETURN(net_socket(domain, type, protocol));
}

SYS_CALL(shutdown) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM2(int, how);

    (void) file;
    (void) how;

    SYS_RETURN(-ENOSYS);
}

SYS_CALL(getsockopt) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM2(int, level);
    SYS_PARAM3(int, optname);
    SYS_PARAM5_VALIDATE(socklen_t *, optlen, validate_write, sizeof(socklen_t));
    SYS_PARAM4_VALIDATE(void *, optval, validate_write, *optlen);

    (void) file;
    (void) level;
    (void) optname;
    (void) optval;
    (void) optlen;

    SYS_RETURN(-ENOSYS);
}

SYS_CALL(setsockopt) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM2(int, level);
    SYS_PARAM3(int, optname);
    SYS_PARAM5(socklen_t, optlen);
    SYS_PARAM4_VALIDATE(const void *, optval, validate_read, optlen);

    SYS_RETURN(net_setsockopt(file, level, optname, optval, optlen));
}

SYS_CALL(getpeername) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3_VALIDATE(socklen_t *, addrlen, validate_write, sizeof(socklen_t));
    SYS_PARAM2_VALIDATE(struct sockaddr *, addr, validate_write, *addrlen);

    (void) file;
    (void) addr;
    (void) addrlen;

    SYS_RETURN(-ENOSYS);
}

SYS_CALL(getsockname) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3_VALIDATE(socklen_t *, addrlen, validate_write, sizeof(socklen_t));
    SYS_PARAM2_VALIDATE(struct sockaddr *, addr, validate_write, *addrlen);

    (void) file;
    (void) addr;
    (void) addrlen;

    SYS_RETURN(-ENOSYS);
}

SYS_CALL(sendto) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3(size_t, len);
    SYS_PARAM2_VALIDATE(const void *, buf, validate_read, len);
    SYS_PARAM4(int, flags);
    SYS_PARAM6(socklen_t, addrlen);
    SYS_PARAM5_VALIDATE(const struct sockaddr *, dest, validate_read_or_null, addrlen);

    SYS_RETURN(net_sendto(file, buf, len, flags, dest, addrlen));
}

SYS_CALL(recvfrom) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_socket);
    SYS_PARAM3(size_t, len);
    SYS_PARAM2_VALIDATE(void *, buf, validate_write, len);
    SYS_PARAM4(int, flags);
    SYS_PARAM6_VALIDATE(socklen_t *, addrlen, validate_write_or_null, sizeof(socklen_t));
    SYS_PARAM5_VALIDATE(struct sockaddr *, source, validate_write_or_null, addrlen ? *addrlen : 0);

    SYS_RETURN(net_recvfrom(file, buf, len, flags, source, addrlen));
}

SYS_CALL(mmap) {
    SYS_BEGIN();

    SYS_PARAM1(void *, addr);
    SYS_PARAM2_VALIDATE(size_t, length, validate_positive, 0);
    SYS_PARAM3(int, prot);
    SYS_PARAM4(int, flags);
    SYS_PARAM5(int, fd);
    SYS_PARAM6(off_t, offset);

    if (flags & MAP_ANONYMOUS) {
        struct vm_region *region = map_region(addr, length, prot, flags & MAP_STACK ? VM_TASK_STACK : VM_PROCESS_ANON_MAPPING);
        if (region == NULL) {
            SYS_RETURN(-ENOMEM);
        }

        struct vm_object *object = vm_create_anon_object(length);
        if (!object) {
            SYS_RETURN(-ENOMEM);
        }

        region->flags |= (flags & MAP_SHARED) ? VM_SHARED : 0;
        region->vm_object = object;
        region->vm_object_offset = 0;

        vm_map_region_with_object(region);
        SYS_RETURN(region->start);
    }

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct file *file = get_current_task()->process->files[fd].file;
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_mmap(addr, length, prot, flags, file, offset));
}

SYS_CALL(munmap) {
    SYS_BEGIN();

    SYS_PARAM1(void *, addr);
    SYS_PARAM2(size_t, length);

    debug_log("munmap: [ %p, %#.16lX, %#.16lX ]\n", addr, length, (uintptr_t) addr + length);

    SYS_RETURN(unmap_range((uintptr_t) addr, length));
}

SYS_CALL(rename) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, old_path, validate_path, -1);
    SYS_PARAM2_VALIDATE(const char *, new_path, validate_path, -1);

    SYS_RETURN(fs_rename(old_path, new_path));
}

SYS_CALL(fcntl) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file_descriptor *, desc, int, get_file_desc);
    SYS_PARAM2(int, command);
    SYS_PARAM3(int, arg);

    SYS_RETURN(fs_fcntl(desc, command, arg));
}

SYS_CALL(fstat) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM2_VALIDATE(struct stat *, stat_struct, validate_write, sizeof(struct stat));

    SYS_RETURN(fs_fstat(file, stat_struct));
}

SYS_CALL(alarm) {
    SYS_BEGIN();

    SYS_PARAM1(unsigned int, seconds);

    struct task *current = get_current_task();

    proc_block_sleep_milliseconds(current, get_time() + seconds * 1000);

    disable_interrupts();
    // Tell signal handling code we were not interruped (as would be implied by this saying -EINTR)
    task_state->cpu_state.rax = 0;

    // Potentially the process should be signaled, but sending it to the thread that made the call
    // to alarm makes more sense anyway.
    signal_task(current->process->pid, current->tid, SIGALRM);
    assert(false);
}

SYS_CALL(fchmod) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM2(mode_t, mode);

    SYS_RETURN(fs_fchmod(file, mode));
}

SYS_CALL(getppid) {
    SYS_BEGIN();

    SYS_RETURN(get_current_task()->process->ppid);
}

SYS_CALL(sigsuspend) {
    SYS_BEGIN_SIGSUSPEND();

    SYS_PARAM1_VALIDATE(const sigset_t *, mask, validate_read, sizeof(sigset_t));

    struct task *current = get_current_task();

    memcpy(&current->saved_sig_mask, &current->sig_mask, sizeof(sigset_t));
    memcpy(&current->sig_mask, mask, sizeof(sigset_t));

    proc_block_custom(current);
}

SYS_CALL(times) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(struct tms *, tms, validate_write, sizeof(struct tms));

    struct task *current = get_current_task();

    // Divide by 10 b/c things are measured in (1 / 100) seconds, not (1 / 1000)
    tms->tms_utime = current->process->times.tms_utime / 10;
    tms->tms_stime = current->process->times.tms_stime / 10;
    tms->tms_cutime = current->process->times.tms_cutime / 10;
    tms->tms_cstime = current->process->times.tms_cstime / 10;

    SYS_RETURN(get_time() / 10);
}

SYS_CALL(create_task) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(struct create_task_args *, args, validate_read, sizeof(struct create_task_args));

    debug_log("Creating task: [ %#.16lX, %#.16lX, %#.16lX, %#.16lX ]\n", args->entry, args->stack_start, (uintptr_t) args->tid_ptr,
              (uintptr_t) args->thread_self_pointer);

    struct task *current = get_current_task();
    proc_bump_process(current->process);

    struct task *task = calloc(1, sizeof(struct task));
    task->process = current->process;
    task->sig_mask = current->sig_mask;
    task->sig_pending = 0;
    task->sched_state = RUNNING_INTERRUPTIBLE;
    task->tid = get_next_tid();
    task->locked_robust_mutex_list_head = args->locked_robust_mutex_list_head;

    task_align_fpu(task);

    task->arch_task.kernel_stack = KERNEL_TASK_STACK_START;
    task->arch_task.setup_kernel_stack = true;
    task->arch_task.user_thread_pointer = args->thread_self_pointer;

    task->arch_task.task_state.stack_state.cs = current->arch_task.task_state.stack_state.cs;
    task->arch_task.task_state.stack_state.rip = args->entry;
    task->arch_task.task_state.stack_state.rflags = current->arch_task.task_state.stack_state.rflags;
    task->arch_task.task_state.stack_state.rsp = args->stack_start - sizeof(uintptr_t);
    *((uintptr_t *) task->arch_task.task_state.stack_state.rsp) = args->push_onto_stack;
    task->arch_task.task_state.stack_state.ss = current->arch_task.task_state.stack_state.ss;
    task->arch_task.task_state.cpu_state.rdi = (uint64_t) args->arg;

    *args->tid_ptr = task->tid;
    sched_add_task(task);

    SYS_RETURN(0);
}

SYS_CALL(exit_task) {
    SYS_BEGIN();

    /* Disable Interrups To Prevent Premature Task Removal, Since Sched State Is Set */
    disable_interrupts();

    struct task *task = get_current_task();
    task->sched_state = EXITING;

    // At this point there should be no locked mutexes in the task, since it explicitly
    // exited. Also, the memory could now be freed and is no longer valid.
    task->locked_robust_mutex_list_head = NULL;

    invalidate_last_saved(task);

    debug_log("Task Exited: [ %d, %d ]\n", task->process->pid, task->tid);
    sys_sched_run_next(task_state);
}

SYS_CALL(os_mutex) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(unsigned int *, __protected, validate_write, sizeof(unsigned int));
    SYS_PARAM2(int, operation);
    SYS_PARAM3(int, expected);
    SYS_PARAM4(int, to_place);
    SYS_PARAM5(int, to_wake);
    SYS_PARAM6_VALIDATE(unsigned int *, to_wait, validate_write_or_null, sizeof(unsigned int));

    struct task *current = get_current_task();

    unsigned int *to_aquire = __protected;
    struct user_mutex *to_unlock = NULL;

    switch (operation) {
        case MUTEX_AQUIRE:
        do_mutex_aquire : {
            struct user_mutex *um = get_user_mutex_locked(to_aquire);
            if (*to_aquire != (unsigned int) expected) {
                // Case where MUTEX_RELEASE occurs before we lock/create the mutex
                unlock_user_mutex(um);
                SYS_RETURN(0);
            }

            add_to_user_mutex_queue(um, current);
            current->sched_state = WAITING;
            if (to_unlock) {
                unlock_user_mutex(to_unlock);
            }
            unlock_user_mutex(um);
            proc_block_custom(current);
            SYS_RETURN(0);
        }
        case MUTEX_WAKE_AND_SET: {
            struct user_mutex *um = get_user_mutex_locked_with_waiters_or_else_write_value(__protected, to_place & ~MUTEX_WAITERS);
            if (um == NULL) {
                SYS_RETURN(0);
            }

            wake_user_mutex(um, to_wake, &to_place);
            *__protected = to_place;
            unlock_user_mutex(um);
            SYS_RETURN(0);
        }
        case MUTEX_RELEASE_AND_WAIT: {
            if (!to_wait) {
                SYS_RETURN(EINVAL);
            }
            to_aquire = to_wait;
            struct user_mutex *um = get_user_mutex_locked_with_waiters_or_else_write_value(__protected, to_place & ~MUTEX_WAITERS);
            if (um == NULL) {
                goto do_mutex_aquire;
            }

            wake_user_mutex(um, to_wake, &to_place);
            to_unlock = um;
            goto do_mutex_aquire;
        }
        default: {
            SYS_RETURN(-EINVAL);
        }
    }
}

SYS_CALL(tgkill) {
    SYS_BEGIN_CAN_SEND_SELF_SIGNALS();

    SYS_PARAM1(int, tgid);
    SYS_PARAM2(int, tid);
    SYS_PARAM3_VALIDATE(int, signum, validate_signal_number, 1);

    struct task *current = get_current_task();

    if (tgid == 0) {
        tgid = current->process->pid;
    }

    if (tid == 0) {
        tid = current->tid;
    }

    SYS_RETURN(signal_task(tgid, tid, signum));
}

SYS_CALL(get_initial_process_info) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(struct initial_process_info *, info, validate_write, sizeof(struct initial_process_info));
    struct task *current = get_current_task();

    info->tls_start = current->process->tls_master_copy_start;
    info->tls_size = current->process->tls_master_copy_size;
    info->tls_alignment = current->process->tls_master_copy_alignment;

    struct vm_region *stack = get_vm_last_region(current->process->process_memory, VM_TASK_STACK);
    info->stack_start = (void *) stack->start;
    info->stack_size = stack->end - stack->start;

    struct vm_region *guard = get_vm_last_region(current->process->process_memory, VM_TASK_STACK_GUARD);
    info->guard_size = guard->end - guard->start;

    info->main_tid = current->tid;

    SYS_RETURN(0);
}

SYS_CALL(set_thread_self_pointer) {
    SYS_BEGIN();

    SYS_PARAM1(void *, thread_self_pointer);

    // NOTE: these list items need to be validated later, when they are
    //       traversed during task clean up
    SYS_PARAM2(struct __locked_robust_mutex_node **, locked_robust_mutex_list_head);

    struct task *current = get_current_task();
    current->arch_task.user_thread_pointer = thread_self_pointer;
    current->locked_robust_mutex_list_head = locked_robust_mutex_list_head;

    set_msr(MSR_FS_BASE, (uint64_t) thread_self_pointer);

    SYS_RETURN(0);
}

SYS_CALL(mprotect) {
    SYS_BEGIN();

    SYS_PARAM1(void *, addr);
    SYS_PARAM2(size_t, length);
    SYS_PARAM3(int, prot);

    SYS_RETURN(map_range_protections((uintptr_t) addr, length, prot));
}

SYS_CALL(sigpending) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(sigset_t *, set, validate_write, sizeof(sigset_t));

    memcpy(set, &get_current_task()->sig_pending, sizeof(sigset_t));
    SYS_RETURN(0);
}

SYS_CALL(sigaltstack) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const stack_t *, stack, validate_read_or_null, sizeof(stack_t));
    SYS_PARAM2_VALIDATE(stack_t *, old_stack, validate_write_or_null, sizeof(stack_t));

    struct process *current_process = get_current_task()->process;

    if (old_stack) {
        memcpy(old_stack, &current_process->alt_stack, sizeof(stack_t));
        if (!(old_stack->ss_flags & __SS_ENABLED)) {
            old_stack->ss_flags |= SS_DISABLE;
        } else {
            old_stack->ss_flags &= ~__SS_ENABLED;
        }
    }

    if (stack) {
        if (stack->ss_size < MINSIGSTKSZ) {
            SYS_RETURN(-ENOMEM);
        }

        memcpy(&current_process->alt_stack, stack, sizeof(stack_t));
        if (!(stack->ss_flags & SS_DISABLE)) {
            current_process->alt_stack.ss_flags |= __SS_ENABLED;
        } else {
            current_process->alt_stack.ss_flags &= ~__SS_ENABLED;
        }
    }

    SYS_RETURN(0);
}

SYS_CALL(pselect) {
    SYS_BEGIN_PSELECT();

    SYS_PARAM1_VALIDATE(int, nfds, validate_positive, 1);
    size_t fd_set_size = ((nfds + sizeof(uint8_t) * CHAR_BIT - 1) / sizeof(uint8_t) / CHAR_BIT);

    SYS_PARAM2_VALIDATE(uint8_t *, readfds, validate_write_or_null, fd_set_size);
    SYS_PARAM3_VALIDATE(uint8_t *, writefds, validate_write_or_null, fd_set_size);
    SYS_PARAM4_VALIDATE(uint8_t *, exceptfds, validate_write_or_null, fd_set_size);
    SYS_PARAM5_VALIDATE(const struct timespec *, timeout, validate_read_or_null, sizeof(struct timespec));
    SYS_PARAM6_VALIDATE(const sigset_t *, sigmask, validate_read_or_null, sizeof(sigset_t));

    struct task *current = get_current_task();

    if (sigmask) {
        memcpy(&current->saved_sig_mask, &current->sig_mask, sizeof(sigset_t));
        memcpy(&current->sig_mask, sigmask, sizeof(sigset_t));
        current->in_sigsuspend = true;
    }

    int count = 0;

    if (nfds > FOPEN_MAX) {
        count = -EINVAL;
        goto pselect_return;
    }

    time_t start = get_time();

    uint8_t *read_fds_found = NULL;
    if (readfds) {
        read_fds_found = alloca(fd_set_size);
        memcpy(read_fds_found, readfds, fd_set_size);
    }

    uint8_t *write_fds_found = NULL;
    if (writefds) {
        write_fds_found = alloca(fd_set_size);
        memcpy(write_fds_found, writefds, fd_set_size);
    }

    uint8_t *except_fds_found = NULL;
    if (exceptfds) {
        except_fds_found = alloca(fd_set_size);
        memcpy(except_fds_found, exceptfds, fd_set_size);
    }

    // NOTE: don't need to take process lock since its undefined behavior to close
    //       a file while another thread is selecting on it
    for (;;) {
        if (read_fds_found) {
            for (size_t i = 0; i < fd_set_size / sizeof(uint8_t); i++) {
                if (read_fds_found[i]) {
                    for (size_t j = 0;
                         read_fds_found[i] && i * sizeof(uint8_t) * CHAR_BIT + j < (size_t) nfds && j < sizeof(uint8_t) * CHAR_BIT; j++) {
                        if (read_fds_found[i] & (1U << j)) {
                            struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j].file;
                            if (fs_is_readable(to_check)) {
                                read_fds_found[i] ^= (1U << j);
                                count++;
                            }
                        }
                    }
                }
            }
        }

        if (write_fds_found) {
            for (size_t i = 0; i < fd_set_size / sizeof(uint8_t); i++) {
                if (write_fds_found[i]) {
                    for (size_t j = 0;
                         write_fds_found[i] && i * sizeof(uint8_t) * CHAR_BIT + j < (size_t) nfds && j < sizeof(uint8_t) * CHAR_BIT; j++) {
                        if (write_fds_found[i] & (1U << j)) {
                            struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j].file;
                            if (fs_is_writable(to_check)) {
                                write_fds_found[i] ^= (1U << j);
                                count++;
                            }
                        }
                    }
                }
            }
        }

        if (except_fds_found) {
            for (size_t i = 0; i < fd_set_size / sizeof(uint8_t); i++) {
                if (except_fds_found[i]) {
                    for (size_t j = 0;
                         except_fds_found[i] && i * sizeof(uint8_t) * CHAR_BIT + j < (size_t) nfds && j < sizeof(uint8_t) * CHAR_BIT; j++) {
                        if (except_fds_found[i] & (1U << j)) {
                            struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j].file;
                            if (fs_is_exceptional(to_check)) {
                                except_fds_found[i] ^= (1U << j);
                                count++;
                            }
                        }
                    }
                }
            }
        }

        if (count > 0) {
            break;
        }

        if (timeout) {
            time_t now = get_time();
            time_t end_time = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000 - (start - now);

            proc_block_select_timeout(current, nfds, create_phys_addr_mapping_from_virt_addr(read_fds_found),
                                      create_phys_addr_mapping_from_virt_addr(write_fds_found),
                                      create_phys_addr_mapping_from_virt_addr(except_fds_found), end_time);

            if (get_time() >= end_time) {
                goto pselect_return;
            }

            continue;
        }

        proc_block_select(current, nfds, create_phys_addr_mapping_from_virt_addr(read_fds_found),
                          create_phys_addr_mapping_from_virt_addr(write_fds_found),
                          create_phys_addr_mapping_from_virt_addr(except_fds_found));
    }

    for (size_t i = 0; i < fd_set_size; i++) {
        if (readfds) {
            readfds[i] ^= read_fds_found[i];
        }
        if (writefds) {
            writefds[i] ^= write_fds_found[i];
        }
        if (exceptfds) {
            exceptfds[i] ^= except_fds_found[i];
        }
    }

pselect_return:
    if (current->in_sigsuspend) {
        SYS_RETURN_RESTORE_SIGMASK(count);
    }

    SYS_RETURN(count);
}

SYS_CALL(yield) {
    SYS_BEGIN();

    __kernel_yield();
    SYS_RETURN(0);
}

SYS_CALL(getuid) {
    SYS_BEGIN();

    SYS_RETURN(get_current_task()->process->uid);
}

SYS_CALL(geteuid) {
    SYS_BEGIN();

    SYS_RETURN(get_current_task()->process->euid);
}

SYS_CALL(getgid) {
    SYS_BEGIN();

    SYS_RETURN(get_current_task()->process->gid);
}

SYS_CALL(getegid) {
    SYS_BEGIN();

    SYS_RETURN(get_current_task()->process->egid);
}

SYS_CALL(setuid) {
    SYS_BEGIN();

    SYS_PARAM1(uid_t, uid);

    struct process *current = get_current_task()->process;
    if (current->uid == 0) {
        current->euid = uid;
        current->uid = uid;
        SYS_RETURN(0);
    }

    if (current->uid != uid) {
        SYS_RETURN(-EPERM);
    }

    current->euid = uid;
    SYS_RETURN(0);
}

SYS_CALL(seteuid) {
    SYS_BEGIN();

    SYS_PARAM1(uid_t, euid);

    struct process *current = get_current_task()->process;
    if (current->uid == 0) {
        current->uid = euid;
        SYS_RETURN(0);
    }

    if (current->euid != euid && current->uid != euid) {
        SYS_RETURN(-EPERM);
    }

    current->euid = euid;
    SYS_RETURN(0);
}

SYS_CALL(setgid) {
    SYS_BEGIN();

    SYS_PARAM1(gid_t, gid);

    struct process *current = get_current_task()->process;
    if (current->gid == 0) {
        current->egid = gid;
        current->gid = gid;
        SYS_RETURN(0);
    }

    if (current->gid != gid) {
        SYS_RETURN(-EPERM);
    }

    current->egid = gid;
    SYS_RETURN(0);
}

SYS_CALL(setegid) {
    SYS_BEGIN();

    SYS_PARAM1(gid_t, egid);

    struct process *current = get_current_task()->process;
    if (current->gid == 0) {
        current->gid = egid;
        SYS_RETURN(0);
    }

    if (current->egid != egid && current->gid != egid) {
        SYS_RETURN(-EPERM);
    }

    current->egid = egid;
    SYS_RETURN(0);
}

SYS_CALL(umask) {
    SYS_BEGIN();

    SYS_PARAM1(mode_t, new_mask);

    struct process *current = get_current_task()->process;
    spin_lock(&current->lock);

    mode_t old_mask = current->umask;
    current->umask = new_mask;

    spin_unlock(&current->lock);
    SYS_RETURN(old_mask);
}

SYS_CALL(uname) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(struct utsname *, buf, validate_write, sizeof(struct utsname));

    strcpy(buf->machine, "x86_64");
    strcpy(buf->sysname, "os_2");
    strcpy(buf->release, "0.0.1");
    strcpy(buf->version, "0");
    strcpy(buf->nodename, "os_2-dev");

    SYS_RETURN(0);
}

SYS_CALL(getsid) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(pid_t, pid, validate_positive, 1);
    if (pid == 0) {
        SYS_RETURN(get_current_task()->process->sid);
    }

    struct process *process = find_by_pid(pid);
    if (!process) {
        SYS_RETURN(-ESRCH);
    }

    SYS_RETURN(process->sid);
}

SYS_CALL(setsid) {
    SYS_BEGIN();

    int ret = 0;

    struct process *current = get_current_task()->process;
    spin_lock(&current->lock);

    if (current->pgid == current->pid) {
        ret = -EPERM;
        goto finish_setsid;
    }

    current->sid = current->pid;
    proc_update_pgid(current->pid, current->pgid);
    current->pgid = current->pid;

finish_setsid:
    spin_unlock(&current->lock);
    SYS_RETURN(ret);
}

SYS_CALL(readlink) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM3(size_t, bufsiz);
    SYS_PARAM2_VALIDATE(char *, buf, validate_write, bufsiz);

    SYS_RETURN(fs_readlink(path, buf, bufsiz));
}

SYS_CALL(lstat) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM2_VALIDATE(struct stat *, stat_struct, validate_write, sizeof(struct stat));

    SYS_RETURN(fs_lstat(path, stat_struct));
}

SYS_CALL(symlink) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, target, validate_path, -1);
    SYS_PARAM2_VALIDATE(const char *, linkpath, validate_path, -1);

    SYS_RETURN(fs_symlink(target, linkpath));
}

SYS_CALL(link) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, oldpath, validate_path, -1);
    SYS_PARAM2_VALIDATE(const char *, newpath, validate_path, -1);

    SYS_RETURN(fs_link(oldpath, newpath));
}

SYS_CALL(chown) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM2(uid_t, uid);
    SYS_PARAM3(gid_t, gid);

    SYS_RETURN(fs_chown(path, uid, gid));
}

SYS_CALL(utimes) {
    SYS_BEGIN();

    SYS_PARAM1(const char *, filename);
    SYS_PARAM2(const struct timeval *, times);

    SYS_RETURN(fs_utimes(filename, times));
}

SYS_CALL(pread) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM3(size_t, count);
    SYS_PARAM2_VALIDATE(void *, buf, validate_write, count);
    SYS_PARAM4_VALIDATE(off_t, offset, validate_positive, 1);

    SYS_RETURN(fs_pread(file, buf, count, offset));
}

SYS_CALL(pwrite) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM3(size_t, count);
    SYS_PARAM2_VALIDATE(const void *, buf, validate_read, count);
    SYS_PARAM4_VALIDATE(off_t, offset, validate_positive, 1);

    SYS_RETURN(fs_pwrite(file, buf, count, offset));
}

SYS_CALL(readv) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM3_VALIDATE(int, item_count, validate_positive, 1);
    SYS_PARAM2_VALIDATE(const struct iovec *, vec, validate_read, item_count * sizeof(struct iovec));

    SYS_RETURN(fs_readv(file, vec, item_count));
}

SYS_CALL(writev) {
    SYS_BEGIN();

    SYS_PARAM1_TRANSFORM(struct file *, file, int, get_file);
    SYS_PARAM3_VALIDATE(int, item_count, validate_positive, 1);
    SYS_PARAM2_VALIDATE(const struct iovec *, vec, validate_read, item_count + sizeof(struct iovec));

    SYS_RETURN(fs_writev(file, vec, item_count));
}

SYS_CALL(realpath) {
    SYS_BEGIN();

    SYS_PARAM1_VALIDATE(const char *, path, validate_path, -1);
    SYS_PARAM3(size_t, buf_max);
    SYS_PARAM2_VALIDATE(char *, buf, validate_write, buf_max);

    struct tnode *tnode;
    {
        int ret = iname(path, 0, &tnode);
        if (ret < 0) {
            SYS_RETURN(ret);
        }
    }

    char *full_path = get_tnode_path(tnode);
    size_t full_path_len = strlen(full_path);
    int ret = 0;

    if (full_path_len >= buf_max) {
        ret = ERANGE;
        goto finish_realpath;
    }

    strcpy(buf, full_path);

finish_realpath:
    free(full_path);
    SYS_RETURN(ret);
}

SYS_CALL(invalid_system_call) {
    SYS_BEGIN();
    SYS_RETURN(-ENOSYS);
}

void arch_system_call_entry(struct task_state *task_state) {
#ifdef SYSCALL_DEBUG
#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x, y, a)    \
    case SC_##x:                        \
        debug_log("syscall: %s\n", #y); \
        break;
    if (get_current_task()->process->should_trace) {
        switch ((enum sc_number) task_state->cpu_state.rdi) {
            ENUMERATE_SYSCALLS
            default:
                debug_log("unknown syscall: [ %d ]\n", (enum sc_number) task_state->cpu_state.rdi);
                break;
        }
    }
#endif /* SYSCALL_DEBUG */

#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x, y, a) \
    case SC_##x:                     \
        arch_sys_##y(task_state);    \
        break;

    switch ((enum sc_number) task_state->cpu_state.rdi) {
        ENUMERATE_SYSCALLS
        default:
            arch_sys_invalid_system_call(task_state);
            break;
    }

    return;
}