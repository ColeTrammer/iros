#include <assert.h>
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
#include <sys/wait.h>

#include <kernel/fs/file.h>
#include <kernel/fs/vfs.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/socket.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

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

#define SYS_BEGIN(task_state)                                                  \
    do {                                                                       \
        get_current_task()->arch_task.user_task_state = (task_state);          \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = -EINTR; \
        get_current_task()->in_kernel = true;                                  \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;             \
        enable_interrupts();                                                   \
    } while (0)

#define SYS_BEGIN_CAN_SEND_SELF_SIGNALS(task_state)                       \
    do {                                                                  \
        get_current_task()->arch_task.user_task_state = (task_state);     \
        get_current_task()->in_kernel = true;                             \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;        \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = 0; \
    } while (0)

#define SYS_BEGIN_SIGSUSPEND(task_state)                                       \
    do {                                                                       \
        get_current_task()->arch_task.user_task_state = (task_state);          \
        get_current_task()->arch_task.user_task_state->cpu_state.rax = -EINTR; \
        get_current_task()->sched_state = RUNNING_UNINTERRUPTIBLE;             \
        get_current_task()->in_kernel = true;                                  \
        get_current_task()->in_sigsuspend = true;                              \
    } while (0)

#define SYS_BEGIN_PSELECT(task_state)                                         \
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

void arch_sys_exit(struct task_state *task_state) {
    SYS_BEGIN(task_state);

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

void arch_sys_sbrk(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    intptr_t increment = task_state->cpu_state.rsi;

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

    SYS_RETURN((uint64_t) res);
}

void arch_sys_fork(struct task_state *task_state) {
    SYS_BEGIN(task_state);

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
    child_process->cwd = malloc(strlen(parent->process->cwd) + 1);
    strcpy(child_process->cwd, parent->process->cwd);
    child_process->pgid = parent->process->pgid;
    child_process->ppid = parent->process->pid;
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
        if (parent->process->files[i]) {
            child_process->files[i] = fs_dup(parent->process->files[i]);
        }
    }

    debug_log("Finishing fork: [ %d:%d ]\n", child->process->pid, child->tid);

    disable_interrupts();
    sched_add_task(child);
    SYS_RETURN(child_process->pid);
}

void arch_sys_open(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;
    int flags = (int) task_state->cpu_state.rdx;
    mode_t mode = (mode_t) task_state->cpu_state.rcx;

    assert(_path != NULL);

    int error = 0;

    struct task *task = get_current_task();
    char *path = get_full_path(task->process->cwd, _path);

    struct file *file = fs_open(path, flags, &error);

    if (file && (flags & O_EXCL)) {
        free(path);
        fs_close(file);
        SYS_RETURN(-EEXIST);
    }

    if (file == NULL) {
        if (flags & O_CREAT) {
            debug_log("Creating file: [ %s ]\n", path);

            error = fs_create(path, mode | S_IFREG);
            if (error) {
                free(path);
                SYS_RETURN((uint64_t) error);
            }

            file = fs_open(path, flags, &error);
            if (file == NULL) {
                free(path);
                SYS_RETURN((uint64_t) error);
            }

            debug_log("Open successful\n");
        } else {
            debug_log("File Not Found: [ %s ]\n", path);
            free(path);
            SYS_RETURN((uint64_t) error);
        }
    }

    /* Should probably be some other error instead */
    if (!(file->flags & FS_DIR) && (flags & O_DIRECTORY)) {
        free(path);
        SYS_RETURN(-EINVAL);
    }

    if (file->flags & FS_DIR && !(flags & O_DIRECTORY)) {
        free(path);
        SYS_RETURN(-EISDIR);
    }

    /* Handle append mode */
    if (flags & O_APPEND) {
        fs_seek(file, 0, SEEK_END);
    }

    for (int i = 0; i < FOPEN_MAX; i++) {
        if (task->process->files[i] == NULL) {
            task->process->files[i] = file;
            free(path);
            SYS_RETURN(i);
        }
    }

    free(path);
    SYS_RETURN(-EMFILE);
}

void arch_sys_read(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    char *buf = (void *) task_state->cpu_state.rdx;
    size_t count = (size_t) task_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct task *task = get_current_task();
    struct file *file = task->process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EINVAL);
    }

    SYS_RETURN((uint64_t) fs_read(file, buf, count));
}

void arch_sys_write(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    void *buf = (void *) task_state->cpu_state.rdx;
    size_t count = (size_t) task_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct task *task = get_current_task();
    struct file *file = task->process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EINVAL);
    }

    SYS_RETURN((uint64_t) fs_write(file, buf, count));
}

void arch_sys_close(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct task *task = get_current_task();
    if (task->process->files[fd] == NULL) {
        SYS_RETURN(-EBADF);
    }
    int error = fs_close(task->process->files[fd]);
    task->process->files[fd] = NULL;

    SYS_RETURN(error);
}

void arch_sys_execve(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *file_name = (const char *) task_state->cpu_state.rsi;
    char **argv = (char **) task_state->cpu_state.rdx;
    char **envp = (char **) task_state->cpu_state.rcx;

    assert(file_name != NULL);
    assert(argv != NULL);
    assert(envp != NULL);

    struct task *current = get_current_task();

    char *path = get_full_path(current->process->cwd, file_name);

    debug_log("Exec Task: [ %d, %s ]\n", current->process->pid, path);

    int error = 0;
    struct file *program = fs_open(path, O_RDONLY, &error);
    if (!program) {
        free(path);
        SYS_RETURN(-ENOENT);
    }

    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);

    if (length == 0) {
        free(path);
        SYS_RETURN(-ENOEXEC);
    }

    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    struct task *task = calloc(1, sizeof(struct task));
    struct process *process = calloc(1, sizeof(struct process));
    task->process = process;

    process->inode_dev = program->device;
    process->inode_id = program->inode_idenifier;

    fs_close(program);

    if (!elf64_is_valid(buffer)) {
        free(path);
        free(buffer);
        SYS_RETURN(-ENOEXEC);
    }

    // Dup open file descriptors
    for (int i = 0; i < FOPEN_MAX; i++) {
        // FIXME: duplicated files should have separate fd_flags (but everything else is the same)
        //        right now this means that FD_CLOEXEC will destroy all duplicated files, so it
        //        cannot be used when doing dup(2) on a builtin stream. This is of course its
        //        primary use case, so FD_CLOEXEC must be ignored or everything will break.
        if (!current->process->files[i] ||
            (/* (current->files[i]->fd_flags & FD_CLOEXEC) || */ (current->process->files[i]->flags & FS_DIR))) {
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
    process->process_memory = kernel_stack;
    process->process_memory = add_vm_region(process->process_memory, process_stack);
    process->process_memory = add_vm_region(process->process_memory, process_guard);
    task->kernel_task = false;
    task->sched_state = RUNNING_INTERRUPTIBLE;
    process->tty = current->process->tty;
    process->cwd = malloc(strlen(current->process->cwd) + 1);
    strcpy(process->cwd, current->process->cwd);
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
    task->arch_task.task_state.stack_state.rsp = map_program_args(process_stack->end, argv, envp);
    task->arch_task.task_state.stack_state.ss = USER_DATA_SELECTOR;

    /* Memset stack to zero so that task can use old one safely (only go until rsp because args are after it). */
    memset((void *) process_stack->start, 0, task->arch_task.task_state.stack_state.rsp - process_stack->start);

    /* Ensure File Name And Args Are Still Mapped */
    soft_remove_paging_structure(current->process->process_memory);

    elf64_load_program(buffer, length, task);
    elf64_map_heap(buffer, task);

    free(path);
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

void arch_sys_waitpid(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    pid_t pid = (pid_t) task_state->cpu_state.rsi;
    int *status = (int *) task_state->cpu_state.rdx;
    int flags = (int) task_state->cpu_state.rcx;

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

void arch_sys_getpid(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    SYS_RETURN(get_current_task()->process->pid);
}

void arch_sys_getcwd(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    char *buffer = (char *) task_state->cpu_state.rsi;
    size_t size = (size_t) task_state->cpu_state.rdx;

    struct task *current = get_current_task();
    if (strlen(current->process->cwd) >= size) {
        SYS_RETURN((uint64_t) NULL);
    }

    strcpy(buffer, current->process->cwd);
    SYS_RETURN((uint64_t) buffer);
}

void arch_sys_chdir(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;

    /* Should probably not do this */
    if (_path[strlen(_path) - 1] == '/') {
        ((char *) _path)[strlen(_path) - 1] = '\0';
    }

    struct task *task = get_current_task();
    char *path = get_full_path(task->process->cwd, _path);

    struct tnode *tnode = iname(path);
    if (!tnode) {
        free(path);
        SYS_RETURN(-ENOENT);
    }

    if (!(tnode->inode->flags & FS_DIR)) {
        free(path);
        SYS_RETURN(-ENOTDIR);
    }

    task->process->cwd = get_tnode_path(tnode);
    debug_log("Chdir: [ %s ]\n", task->process->cwd);

    free(path);
    SYS_RETURN(0);
}

void arch_sys_stat(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;
    void *stat_struct = (void *) task_state->cpu_state.rdx;

    struct task *current = get_current_task();
    char *path = get_full_path(current->process->cwd, _path);

    int ret = fs_stat(path, stat_struct);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_lseek(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    off_t offset = (off_t) task_state->cpu_state.rdx;
    int whence = (int) task_state->cpu_state.rcx;

    struct task *task = get_current_task();
    struct file *file = task->process->files[fd];
    assert(file);

    SYS_RETURN((uint64_t) fs_seek(file, offset, whence));
}

void arch_sys_ioctl(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    unsigned long request = (unsigned long) task_state->cpu_state.rdx;
    void *argp = (void *) task_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct task *task = get_current_task();
    struct file *file = task->process->files[fd];

    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN((uint64_t) fs_ioctl(file, request, argp));
}

void arch_sys_ftruncate(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    off_t length = (off_t) task_state->cpu_state.rdx;

    if (fd < 0 || fd >= FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct task *task = get_current_task();
    struct file *file = task->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN((uint64_t) fs_truncate(file, length));
}

void arch_sys_gettimeofday(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    struct timeval *tv = (struct timeval *) task_state->cpu_state.rsi;
    struct timezone *tz = (struct timezone *) task_state->cpu_state.rdx;

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

void arch_sys_mkdir(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *pathname = (const char *) task_state->cpu_state.rsi;
    mode_t mode = (mode_t) task_state->cpu_state.rdx;

    struct task *current = get_current_task();
    char *path = get_full_path(current->process->cwd, pathname);

    int ret = fs_mkdir(path, mode);

    free(path);
    SYS_RETURN((uint64_t) ret);
}

void arch_sys_dup2(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int oldfd = (int) task_state->cpu_state.rsi;
    int newfd = (int) task_state->cpu_state.rdx;

#ifdef DUP_DEBUG
    debug_log("Dup: [ %d, %d ]\n", oldfd, newfd);
#endif /* DUP_DEBUG */

    if (oldfd < 0 || oldfd >= FOPEN_MAX || newfd < 0 || newfd >= FOPEN_MAX) {
        SYS_RETURN((uint64_t) -EBADFD);
    }

    struct task *task = get_current_task();
    if (task->process->files[newfd] != NULL) {
        int ret = fs_close(task->process->files[newfd]);
        if (ret != 0) {
            SYS_RETURN((uint64_t) ret);
        }
    }

    task->process->files[newfd] = fs_dup(task->process->files[oldfd]);
    SYS_RETURN(0);
}

void arch_sys_pipe(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int *pipefd = (int *) task_state->cpu_state.rsi;

    struct file *pipe_files[2];
    int ret = fs_create_pipe(pipe_files);
    if (ret != 0) {
        SYS_RETURN(ret);
    }

    struct task *task = get_current_task();
    int j = 0;
    for (int i = 0; j < 2 && i < FOPEN_MAX; i++) {
        if (task->process->files[i] == NULL) {
            debug_log("Allocating pipe to: [ %d, %d ]\n", i, j);
            task->process->files[i] = pipe_files[j];
            pipefd[j] = i;
            j++;
        }
    }

    assert(j == 2);

    SYS_RETURN(0);
}

void arch_sys_unlink(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;

    struct task *current = get_current_task();
    char *path = get_full_path(current->process->cwd, _path);

    int ret = fs_unlink(path);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_rmdir(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;

    struct task *current = get_current_task();
    char *path = get_full_path(current->process->cwd, _path);

    int ret = fs_rmdir(path);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_chmod(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;
    mode_t mode = (mode_t) task_state->cpu_state.rdx;

    struct task *task = get_current_task();
    char *path = get_full_path(task->process->cwd, _path);

    int ret = fs_chmod(path, mode);

    free(path);
    SYS_RETURN(ret);
}

void arch_sys_kill(struct task_state *task_state) {
    SYS_BEGIN_CAN_SEND_SELF_SIGNALS(task_state);

    pid_t pid = (pid_t) task_state->cpu_state.rsi;
    int signum = (int) task_state->cpu_state.rdx;

    struct task *current = get_current_task();

    // pid -1 is not yet implemented
    if (signum < 0 || signum >= _NSIG || pid == -1) {
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

void arch_sys_setpgid(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    pid_t pid = (pid_t) task_state->cpu_state.rsi;
    pid_t pgid = (pid_t) task_state->cpu_state.rdx;

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

void arch_sys_sigaction(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int signum = (int) task_state->cpu_state.rsi;
    const struct sigaction *act = (const struct sigaction *) task_state->cpu_state.rdx;
    struct sigaction *old_act = (struct sigaction *) task_state->cpu_state.rcx;

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

void arch_sys_sigreturn(struct task_state *task_state) {
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

void arch_sys_sigprocmask(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int how = (int) task_state->cpu_state.rsi;
    const sigset_t *set = (const sigset_t *) task_state->cpu_state.rdx;
    sigset_t *old = (sigset_t *) task_state->cpu_state.rcx;

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

void arch_sys_dup(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int oldfd = (int) task_state->cpu_state.rsi;

    struct task *current = get_current_task();
    if (current->process->files[oldfd] == NULL) {
        SYS_RETURN(-EBADF);
    }

    // Should lock task to prevent races
    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (current->process->files[i] == NULL) {
            current->process->files[i] = fs_dup(current->process->files[oldfd]);
#ifdef DUP_DEBUG
            debug_log("Dup: [ %d, %lu ]\n", oldfd, i);
#endif /* DUP_DEBUG */
            SYS_RETURN(i);
        }
    }

    SYS_RETURN(-EMFILE);
}

void arch_sys_getpgid(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    pid_t pid = (pid_t) task_state->cpu_state.rsi;
    if (pid == 0) {
        pid = get_current_task()->process->pid;
    }

    SYS_RETURN(proc_get_pgid(pid));
}

void arch_sys_sleep(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    unsigned int seconds = (unsigned int) task_state->cpu_state.rsi;

    debug_log("Sleeping: [ %u ]\n", seconds);

    struct task *current = get_current_task();

    time_t end_time = get_time() + seconds * 1000;

    proc_block_sleep_milliseconds(current, end_time);

    time_t now = get_time();
    SYS_RETURN(now < end_time ? end_time - now : 0);
}

void arch_sys_access(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_path = (const char *) task_state->cpu_state.rsi;
    int mode = (int) task_state->cpu_state.rdx;

    struct task *current = get_current_task();
    char *path = get_full_path(current->process->cwd, _path);

    int ret = fs_access(path, mode);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_accept4(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr *) task_state->cpu_state.rdx;
    socklen_t *addrlen = (socklen_t *) task_state->cpu_state.rcx;
    int flags = (int) task_state->cpu_state.r8;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_accept(file, addr, addrlen, flags));
}

void arch_sys_bind(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    const struct sockaddr *addr = (const struct sockaddr *) task_state->cpu_state.rdx;
    socklen_t addrlen = (socklen_t) task_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_bind(file, addr, addrlen));
}

void arch_sys_connect(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr *) task_state->cpu_state.rdx;
    socklen_t addrlen = (socklen_t) task_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_connect(file, addr, addrlen));
}

void arch_sys_listen(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    int backlog = (int) task_state->cpu_state.rdx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_listen(file, backlog));
}

void arch_sys_socket(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int domain = (int) task_state->cpu_state.rsi;
    int type = (int) task_state->cpu_state.rdx;
    int protocol = (int) task_state->cpu_state.rcx;

    SYS_RETURN(net_socket(domain, type, protocol));
}

void arch_sys_shutdown(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    int how = (int) task_state->cpu_state.rdx;

    (void) fd;
    (void) how;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_getsockopt(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    int level = (int) task_state->cpu_state.rdx;
    int optname = (int) task_state->cpu_state.rcx;
    const void *optval = (const void *) task_state->cpu_state.r8;
    socklen_t *optlen = (socklen_t *) task_state->cpu_state.r9;

    (void) fd;
    (void) level;
    (void) optname;
    (void) optval;
    (void) optlen;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_setsockopt(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    int level = (int) task_state->cpu_state.rdx;
    int optname = (int) task_state->cpu_state.rcx;
    const void *optval = (const void *) task_state->cpu_state.r8;
    socklen_t optlen = (socklen_t) task_state->cpu_state.r9;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_setsockopt(file, level, optname, optval, optlen));
}

void arch_sys_getpeername(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr *) task_state->cpu_state.rdx;
    socklen_t *addrlen = (socklen_t *) task_state->cpu_state.rcx;

    (void) fd;
    (void) addr;
    (void) addrlen;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_getsockname(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr *) task_state->cpu_state.rdx;
    socklen_t *addrlen = (socklen_t *) task_state->cpu_state.rcx;

    (void) fd;
    (void) addr;
    (void) addrlen;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_sendto(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    const void *buf = (const void *) task_state->cpu_state.rdx;
    size_t len = (size_t) task_state->cpu_state.rcx;
    int flags = (int) task_state->cpu_state.r8;
    const struct sockaddr *dest = (const struct sockaddr *) task_state->cpu_state.r9;
    socklen_t addrlen = (socklen_t) task_state->cpu_state.r10;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_sendto(file, buf, len, flags, dest, addrlen));
}

void arch_sys_recvfrom(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    void *buf = (void *) task_state->cpu_state.rdx;
    size_t len = (size_t) task_state->cpu_state.rcx;
    int flags = (int) task_state->cpu_state.r8;
    struct sockaddr *source = (struct sockaddr *) task_state->cpu_state.r9;
    socklen_t *addrlen = (socklen_t *) task_state->cpu_state.r10;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_task()->process->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_recvfrom(file, buf, len, flags, source, addrlen));
}

void arch_sys_mmap(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    void *addr = (void *) task_state->cpu_state.rsi;
    size_t length = (size_t) task_state->cpu_state.rdx;
    int prot = (int) task_state->cpu_state.rcx;
    int flags = (int) task_state->cpu_state.r8;
    int fd = (int) task_state->cpu_state.r9;
    off_t offset = (off_t) task_state->cpu_state.r10;

    if (length == 0) {
        SYS_RETURN(-EINVAL);
    }

    if (flags & MAP_ANONYMOUS) {
        if (!(flags & MAP_PRIVATE)) {
            SYS_RETURN(-EINVAL);
        }

        struct vm_region *region = map_region(addr, length, prot, flags & MAP_STACK ? VM_TASK_STACK : VM_PROCESS_ANON_MAPPING);
        if (region == NULL) {
            SYS_RETURN(-ENOMEM);
        }

        SYS_RETURN(region->start);
    }

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct file *file = get_current_task()->process->files[fd];

    SYS_RETURN(fs_mmap(addr, length, prot, flags, file, offset));
}

void arch_sys_munmap(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    void *addr = (void *) task_state->cpu_state.rsi;
    size_t length = (size_t) task_state->cpu_state.rdx;

    debug_log("munmap: [ %p, %#.16lX, %#.16lX ]\n", addr, length, (uintptr_t) addr + length);

    SYS_RETURN(unmap_range((uintptr_t) addr, length));
}

void arch_sys_rename(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const char *_old_path = (const char *) task_state->cpu_state.rsi;
    const char *_new_path = (const char *) task_state->cpu_state.rdx;

    if (_old_path == NULL || _new_path == NULL) {
        SYS_RETURN(-EINVAL);
    }

    struct task *current = get_current_task();
    char *old_path = get_full_path(current->process->cwd, _old_path);
    char *new_path = get_full_path(current->process->cwd, _new_path);

    int ret = fs_rename(old_path, new_path);

    free(old_path);
    free(new_path);

    SYS_RETURN(ret);
}

void arch_sys_fcntl(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    int command = (int) task_state->cpu_state.rdx;
    int arg = (int) task_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct task *current = get_current_task();
    struct file *file = current->process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_fcntl(file, command, arg));
}

void arch_sys_fstat(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    struct stat *stat_struct = (struct stat *) task_state->cpu_state.rdx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct task *current = get_current_task();
    struct file *file = current->process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_fstat(file, stat_struct));
}

void arch_sys_alarm(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    unsigned int seconds = (unsigned int) task_state->cpu_state.rsi;

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

void arch_sys_fchmod(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    int fd = (int) task_state->cpu_state.rsi;
    mode_t mode = (mode_t) task_state->cpu_state.rdx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct task *current = get_current_task();
    struct file *file = current->process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_fchmod(file, mode));
}

void arch_sys_getppid(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    SYS_RETURN(get_current_task()->process->ppid);
}

void arch_sys_sigsuspend(struct task_state *task_state) {
    SYS_BEGIN_SIGSUSPEND(task_state);

    struct task *current = get_current_task();
    const sigset_t *mask = (const sigset_t *) task_state->cpu_state.rsi;
    if (mask == NULL) {
        current->in_sigsuspend = false;
        SYS_RETURN(-EFAULT);
    }

    memcpy(&current->saved_sig_mask, &current->sig_mask, sizeof(sigset_t));
    memcpy(&current->sig_mask, mask, sizeof(sigset_t));

    proc_block_custom(current);
}

void arch_sys_times(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    struct tms *tms = (struct tms *) task_state->cpu_state.rsi;
    if (tms == NULL) {
        SYS_RETURN(-EFAULT);
    }

    struct task *current = get_current_task();

    // Divide by 10 b/c things are measured in (1 / 100) seconds, not (1 / 1000)
    tms->tms_utime = current->process->times.tms_utime / 10;
    tms->tms_stime = current->process->times.tms_stime / 10;
    tms->tms_cutime = current->process->times.tms_cutime / 10;
    tms->tms_cstime = current->process->times.tms_cstime / 10;

    SYS_RETURN(get_time() / 10);
}

void arch_sys_create_task(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    struct create_task_args *args = (struct create_task_args *) task_state->cpu_state.rsi;

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

void arch_sys_exit_task(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    /* Disable Interrups To Prevent Premature Task Removal, Since Sched State Is Set */
    disable_interrupts();

    struct task *task = get_current_task();
    task->sched_state = EXITING;

    invalidate_last_saved(task);

    debug_log("Task Exited: [ %d, %d ]\n", task->process->pid, task->tid);
    sys_sched_run_next(task_state);
}

void arch_sys_os_mutex(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    struct task *current = get_current_task();

    int *__protected = (int *) task_state->cpu_state.rsi;
    int operation = (int) task_state->cpu_state.rdx;
    int expected = (int) task_state->cpu_state.rcx;
    int to_place = (int) task_state->cpu_state.r8;
    int to_wake = (int) task_state->cpu_state.r9;
    int *to_wait = (int *) task_state->cpu_state.r10;

    int *to_aquire = __protected;
    struct user_mutex *to_unlock = NULL;

    switch (operation) {
        case MUTEX_AQUIRE:
        do_mutex_aquire : {
            struct user_mutex *um = get_user_mutex_locked(to_aquire);
            if (*to_aquire != expected) {
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
        case MUTEX_RELEASE: {
            struct user_mutex *um = get_user_mutex_locked_with_waiters_or_else_write_value(__protected, to_place);
            if (um == NULL) {
                // There was no one waiting
#ifdef USER_MUTEX_DEBUG
                debug_log("no one waiting\n");
#endif /* USER_MUTEX_DEBUG */
                SYS_RETURN(0);
            }

            wake_user_mutex(um, to_wake);
            unlock_user_mutex(um);
            SYS_RETURN(0);
        }
        case MUTEX_WAKE_AND_SET: {
            struct user_mutex *um = get_user_mutex_locked_with_waiters_or_else_write_value(__protected, to_place);
            if (um == NULL) {
                SYS_RETURN(0);
            }

            *__protected = to_place;
            wake_user_mutex(um, to_wake);
            unlock_user_mutex(um);
            SYS_RETURN(0);
        }
        case MUTEX_RELEASE_AND_WAIT: {
            to_aquire = to_wait;
            struct user_mutex *um = get_user_mutex_locked_with_waiters_or_else_write_value(__protected, to_place);
            if (um == NULL) {
                goto do_mutex_aquire;
            }

            wake_user_mutex(um, to_wake);
            to_unlock = um;
            goto do_mutex_aquire;
        }
        default: {
            SYS_RETURN(EINVAL);
        }
    }
}

void arch_sys_tgkill(struct task_state *task_state) {
    SYS_BEGIN_CAN_SEND_SELF_SIGNALS(task_state);

    int tgid = (int) task_state->cpu_state.rsi;
    int tid = (int) task_state->cpu_state.rdx;
    int signum = (int) task_state->cpu_state.rcx;

    struct task *current = get_current_task();

    if (signum < 0 || signum >= _NSIG) {
        SYS_RETURN(EINVAL);
    }

    if (tgid == 0) {
        tgid = current->process->pid;
    }

    if (tid == 0) {
        tid = current->tid;
    }

    SYS_RETURN(signal_task(tgid, tid, signum));
}

void arch_sys_get_initial_process_info(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    struct initial_process_info *info = (struct initial_process_info *) task_state->cpu_state.rsi;
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

void arch_sys_set_thread_self_pointer(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    void *thread_self_pointer = (void *) task_state->cpu_state.rsi;
    get_current_task()->arch_task.user_thread_pointer = thread_self_pointer;

    set_msr(MSR_FS_BASE, (uint64_t) thread_self_pointer);

    SYS_RETURN(0);
}

void arch_sys_mprotect(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    void *addr = (void *) task_state->cpu_state.rsi;
    size_t length = (size_t) task_state->cpu_state.rdx;
    int prot = (int) task_state->cpu_state.rcx;

    SYS_RETURN(map_range_protections((uintptr_t) addr, length, prot));
}

void arch_sys_sigpending(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    sigset_t *set = (sigset_t *) task_state->cpu_state.rsi;
    if (set == NULL) {
        SYS_RETURN(-EFAULT);
    }

    memcpy(set, &get_current_task()->sig_pending, sizeof(sigset_t));
    SYS_RETURN(0);
}

void arch_sys_sigaltstack(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    const stack_t *stack = (const stack_t *) task_state->cpu_state.rsi;
    stack_t *old_stack = (stack_t *) task_state->cpu_state.rdx;

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

void arch_sys_pselect(struct task_state *task_state) {
    SYS_BEGIN_PSELECT(task_state);

    int nfds = (int) task_state->cpu_state.rsi;
    uint8_t *readfds = (uint8_t *) task_state->cpu_state.rdx;
    uint8_t *writefds = (uint8_t *) task_state->cpu_state.rcx;
    uint8_t *exceptfds = (uint8_t *) task_state->cpu_state.r8;
    const struct timespec *timeout = (const struct timespec *) task_state->cpu_state.r9;
    const sigset_t *sigmask = (const sigset_t *) task_state->cpu_state.r10;

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

    size_t fd_set_size = ((nfds + sizeof(uint8_t) * CHAR_BIT - 1) / sizeof(uint8_t) / CHAR_BIT);

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
                            struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j];
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
                            struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j];
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
                            struct file *to_check = current->process->files[i * sizeof(uint8_t) * CHAR_BIT + j];
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

void arch_sys_yield(struct task_state *task_state) {
    SYS_BEGIN(task_state);

    __kernel_yield();
    SYS_RETURN(0);
}

void arch_sys_invalid_system_call(struct task_state *task_state) {
    SYS_BEGIN(task_state);
    SYS_RETURN(-ENOSYS);
}

void arch_system_call_entry(struct task_state *task_state) {
#ifdef SYSCALL_DEBUG
#undef __ENUMERATE_SYSCALL
#define __ENUMERATE_SYSCALL(x, y, a)    \
    case SC_##x:                        \
        debug_log("syscall: %s\n", #y); \
        break;
    switch ((enum sc_number) task_state->cpu_state.rdi) {
        ENUMERATE_SYSCALLS
        default:
            debug_log("unknown syscall: [ %d ]\n", (enum sc_number) task_state->cpu_state.rdi);
            break;
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