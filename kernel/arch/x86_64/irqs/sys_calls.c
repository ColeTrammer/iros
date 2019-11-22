#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/elf64.h>
#include <kernel/sched/process_sched.h>
#include <kernel/fs/file.h>
#include <kernel/fs/vfs.h>
#include <kernel/proc/process_state.h>
#include <kernel/net/socket.h>

#include <kernel/irqs/handlers.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/arch/x86_64/proc/process.h>
#include <kernel/hal/x86_64/gdt.h>

// #define DUP_DEBUG
// #define SET_PGID_DEBUG
// #define SIGACTION_DEBUG
// #define WAIT_PID_DEBUG

#define SYS_BEGIN(process_state)                                                                                        \
    do {                                                                                                                \
        memcpy(&get_current_process()->arch_process.user_process_state, (process_state), sizeof(struct process_state)); \
        get_current_process()->in_kernel = true;                                                                        \
        enable_interrupts();                                                                                            \
    } while (0)

#define SYS_BEGIN_CAN_SEND_SELF_SIGNALS(process_state)                                                                  \
    do {                                                                                                                \
        memcpy(&get_current_process()->arch_process.user_process_state, (process_state), sizeof(struct process_state)); \
        get_current_process()->can_send_self_signals = true;                                                            \
    } while (0)

#define SYS_RETURN(val)                                       \
    do {                                                      \
        uint64_t _val = (uint64_t) val;                       \
        disable_interrupts();                                 \
        process_state->cpu_state.rax = (_val);                \
        get_current_process()->in_kernel = false;             \
        get_current_process()->can_send_self_signals = false; \
        return;                                               \
    } while (0)

void arch_sys_exit(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    /* Disable Interrups To Prevent Premature Process Removal, Since Sched State Is Set */
    disable_interrupts();

    struct process *process = get_current_process();
    process->sched_state = EXITING;

    invalidate_last_saved(process);

    int exit_code = (int) process_state->cpu_state.rsi;
    proc_add_message(process->pid, proc_create_message(STATE_EXITED, exit_code));
    debug_log("Process Exited: [ %d, %d ]\n", process->pid, exit_code);

    sys_sched_run_next(process_state);
}

void arch_sys_sbrk(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    intptr_t increment = process_state->cpu_state.rsi;

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

void arch_sys_fork(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    struct process *parent = get_current_process();
    struct process *child = calloc(1, sizeof(struct process));
    child->pid = get_next_pid();
    child->sched_state = READY;
    child->kernel_process = false;
    child->process_memory = clone_process_vm();
    child->tty = parent->tty;

    debug_log("Forking Process: [ %d ]\n", parent->pid);

    memcpy(&child->arch_process.process_state, process_state, sizeof(struct process_state));
    child->arch_process.process_state.cpu_state.rax = 0;
    child->arch_process.cr3 = clone_process_paging_structure();
    child->arch_process.kernel_stack = KERNEL_PROC_STACK_START;
    child->arch_process.setup_kernel_stack = true;
    child->cwd = malloc(strlen(parent->cwd) + 1);
    child->pgid = parent->pgid;
    child->ppid = parent->pid;
    child->sig_pending = 0;
    child->sig_mask = parent->sig_mask;
    child->inode_dev = parent->inode_dev;
    child->inode_id = parent->inode_id;
    memcpy(&child->sig_state, &parent->sig_state, sizeof(struct sigaction) * _NSIG);
    strcpy(child->cwd, parent->cwd);

    proc_align_fpu(child);

    // Clone fpu if necessary
    if (parent->fpu.saved) {
        child->fpu.saved = true;
        memcpy(child->fpu.aligned_state, parent->fpu.aligned_state, sizeof(child->fpu.raw_fpu_state.image));
    }

    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (parent->files[i]) {
            child->files[i] = fs_dup(parent->files[i]);
        }
    }

    disable_interrupts();
    sched_add_process(child);
    SYS_RETURN(child->pid);
}

void arch_sys_open(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;
    int flags = (int) process_state->cpu_state.rdx;
    mode_t mode = (mode_t) process_state->cpu_state.rcx;
    
    assert(_path != NULL);

    int error = 0;

    struct process *process = get_current_process();
    char *path = get_full_path(process->cwd, _path);

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
        if (process->files[i] == NULL) {
            process->files[i] = file;
            free(path);
            SYS_RETURN(i);
        }
    }

    free(path);
    SYS_RETURN(-EMFILE);
}

void arch_sys_read(struct process_state *process_state)  {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    char *buf = (void*) process_state->cpu_state.rdx;
    size_t count = (size_t) process_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EINVAL);
    }

    SYS_RETURN((uint64_t) fs_read(file, buf, count));
}

void arch_sys_write(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    void *buf = (void*) process_state->cpu_state.rdx;
    size_t count = (size_t) process_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EINVAL);
    }

    SYS_RETURN((uint64_t) fs_write(file, buf, count));
}

void arch_sys_close(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct process *process = get_current_process();
    if (process->files[fd] == NULL) {
        SYS_RETURN(-EBADF);
    }
    int error = fs_close(process->files[fd]);
    process->files[fd] = NULL;

    SYS_RETURN(error);
}

void arch_sys_execve(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *file_name = (const char*) process_state->cpu_state.rsi;
    char **argv = (char**) process_state->cpu_state.rdx;
    char **envp = (char**) process_state->cpu_state.rcx;

    assert(file_name != NULL);
    assert(argv != NULL);
    assert(envp != NULL);

    struct process *current = get_current_process();

    char *path = get_full_path(current->cwd, file_name);

    debug_log("Exec Process: [ %d, %s ]\n", current->pid, path);

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

    struct process *process = calloc(1, sizeof(struct process));
    process->inode_dev = program->device;
    process->inode_id = program->inode_idenifier;

    fs_close(program);

    if (!elf64_is_valid(buffer)) {
        free(path);
        free(buffer);
        SYS_RETURN(-ENOEXEC);
    }

    // Dup open file descriptors
    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (!current->files[i] || ((current->files[i]->fd_flags & FD_CLOEXEC) || (current->files[i]->flags & FS_DIR))) {
            // NOTE: the files will be closed by the `free_process` function
            continue;
        }

        process->files[i] = fs_dup(current->files[i]);
    }

    /* Clone vm_regions so that they can be freed later */
    struct vm_region *__process_stack = get_vm_region(current->process_memory, VM_PROCESS_STACK);
    struct vm_region *__kernel_stack = get_vm_region(current->process_memory, VM_KERNEL_STACK);

    struct vm_region *process_stack = calloc(1, sizeof(struct vm_region));
    struct vm_region *kernel_stack = calloc(1, sizeof(struct vm_region));

    memcpy(process_stack, __process_stack, sizeof(struct vm_region));
    memcpy(kernel_stack, __kernel_stack, sizeof(struct vm_region));

    process->pid = current->pid;
    process->pgid = current->pgid;
    process->ppid = current->ppid;
    process->process_memory = kernel_stack;
    process->process_memory = add_vm_region(process->process_memory, process_stack);
    process->kernel_process = false;
    process->sched_state = READY;
    process->tty = current->tty;
    process->cwd = malloc(strlen(current->cwd) + 1);
    strcpy(process->cwd, current->cwd);
    process->next = NULL;
    process->sig_mask = current->sig_mask;

    // Clone only signal dispositions that don't have a handler
    for (int i = 0; i < _NSIG; i++) {
        if ((uintptr_t) current->sig_state[i].sa_handler <= (uintptr_t) SIG_IGN) {
            memcpy(&process->sig_state[i], &current->sig_state[i], sizeof(struct sigaction));
        }
    }

    process->arch_process.cr3 = get_cr3();
    process->arch_process.kernel_stack = KERNEL_PROC_STACK_START;

    struct virt_page_info *info = calloc(1, sizeof(struct virt_page_info));
    memcpy(info, current->arch_process.kernel_stack_info, sizeof(struct virt_page_info));

    process->arch_process.kernel_stack_info = info;
    process->arch_process.setup_kernel_stack = false;

    proc_align_fpu(process);

    process->arch_process.process_state.cpu_state.rbp = KERNEL_PROC_STACK_START;
    process->arch_process.process_state.stack_state.rip = elf64_get_entry(buffer);
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    process->arch_process.process_state.stack_state.rflags = get_rflags() | INTERRUPS_ENABLED_FLAG;
    process->arch_process.process_state.stack_state.rsp = map_program_args(process_stack->end, argv, envp);
    process->arch_process.process_state.stack_state.ss = USER_DATA_SELECTOR;

    /* Memset stack to zero so that process can use old one safely (only go until rsp because args are after it). */
    memset((void*) process_stack->start, 0, process->arch_process.process_state.stack_state.rsp - process_stack->start);

    /* Ensure File Name And Args Are Still Mapped */
    soft_remove_paging_structure(current->process_memory);

    elf64_load_program(buffer, length, process);
    elf64_map_heap(buffer, process);

    free(path);
    free(buffer);

    /* Disable Preemption So That Nothing Goes Wrong When Removing Ourselves (We Don't Want To Remove Ourselves From The List And Then Be Interrupted) */
    disable_interrupts();

    sched_remove_process(current);
    invalidate_last_saved(current);
    free_process(current, false);
    sched_add_process(process);

    sys_sched_run_next(&process->arch_process.process_state);
}

void arch_sys_waitpid(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    pid_t pid = (pid_t) process_state->cpu_state.rsi;
    int *status = (int*) process_state->cpu_state.rdx;
    int flags = (int) process_state->cpu_state.rcx;

    struct process *current = get_current_process();

    if (pid == 0) {
        pid = -current->pgid;
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
            found_pid = proc_consume_message_by_parent(current->pid, &m);
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
                yield();
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

void arch_sys_getpid(struct process_state *process_state) {    
    SYS_BEGIN(process_state);

    SYS_RETURN(get_current_process()->pid);
}

void arch_sys_getcwd(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    char *buffer = (char*) process_state->cpu_state.rsi;
    size_t size = (size_t) process_state->cpu_state.rdx;

    struct process *current = get_current_process();
    if (strlen(current->cwd) >= size) {
        SYS_RETURN((uint64_t) NULL);
    }

    strcpy(buffer, current->cwd);
    SYS_RETURN((uint64_t) buffer);
}

void arch_sys_chdir(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;

    /* Should probably not do this */
    if (_path[strlen(_path) - 1] == '/') {
        ((char*) _path)[strlen(_path) - 1] = '\0';
    }

    struct process *process = get_current_process();
    char *path = get_full_path(process->cwd, _path);

    struct tnode *tnode = iname(path);
    if (!tnode) {
        free(path);
        SYS_RETURN(-ENOENT);
    }

    if (!(tnode->inode->flags & FS_DIR)) {
        free(path);
        SYS_RETURN(-ENOTDIR);
    }

    process->cwd = get_tnode_path(tnode);
    debug_log("Chdir: [ %s ]\n", process->cwd);

    free(path);
    SYS_RETURN(0);
}

void arch_sys_stat(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;
    void *stat_struct = (void*) process_state->cpu_state.rdx;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_stat(path, stat_struct);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_lseek(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    off_t offset = (off_t) process_state->cpu_state.rdx;
    int whence = (int) process_state->cpu_state.rcx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file);

    SYS_RETURN((uint64_t) fs_seek(file, offset, whence));
}

void arch_sys_ioctl(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    unsigned long request = (unsigned long) process_state->cpu_state.rdx;
    void *argp = (void*) process_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct process *process = get_current_process();
    struct file *file = process->files[fd];

    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN((uint64_t) fs_ioctl(file, request, argp));
}

void arch_sys_ftruncate(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    off_t length = (off_t) process_state->cpu_state.rdx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file);

    SYS_RETURN((uint64_t) fs_truncate(file, length));
}

void arch_sys_gettimeofday(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    struct timeval *tv = (struct timeval*) process_state->cpu_state.rsi;
    struct timezone *tz = (struct timezone*) process_state->cpu_state.rdx;

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

void arch_sys_mkdir(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *pathname = (const char*) process_state->cpu_state.rsi;
    mode_t mode = (mode_t) process_state->cpu_state.rdx;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, pathname);

    int ret = fs_mkdir(path, mode);

    free(path);
    SYS_RETURN((uint64_t) ret);
}

void arch_sys_dup2(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int oldfd = (int) process_state->cpu_state.rsi;
    int newfd = (int) process_state->cpu_state.rdx;

#ifdef DUP_DEBUG
    debug_log("Dup: [ %d, %d ]\n", oldfd, newfd);
#endif /* DUP_DEBUG */

    if (oldfd < 0 || oldfd >= FOPEN_MAX || newfd < 0 || newfd >= FOPEN_MAX) {
        SYS_RETURN((uint64_t) -EBADFD);
    }

    struct process *process = get_current_process();
    if (process->files[newfd] != NULL) {
        int ret = fs_close(process->files[newfd]);
        if (ret != 0) {
            SYS_RETURN((uint64_t) ret);
        }
    }

    process->files[newfd] = fs_dup(process->files[oldfd]);
    SYS_RETURN(0);
}

void arch_sys_pipe(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int *pipefd = (int*) process_state->cpu_state.rsi;

    struct file *pipe_files[2];
    int ret = fs_create_pipe(pipe_files);
    if (ret != 0) {
        SYS_RETURN(ret);
    }

    struct process *process = get_current_process();
    int j = 0;
    for (int i = 0; j < 2 && i < FOPEN_MAX; i++) {
        if (process->files[i] == NULL) {
            debug_log("Allocating pipe to: [ %d, %d ]\n", i, j);
            process->files[i] = pipe_files[j];
            pipefd[j] = i;
            j++;
        }
    }

    assert(j == 2);

    SYS_RETURN(0);
}

void arch_sys_unlink(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_unlink(path);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_rmdir(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_rmdir(path);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_chmod(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;
    mode_t mode = (mode_t) process_state->cpu_state.rdx;

    struct process *process = get_current_process();
    char *path = get_full_path(process->cwd, _path);

    int ret = fs_chmod(path, mode);

    free(path);
    SYS_RETURN(ret);
}

void arch_sys_kill(struct process_state *process_state) {
    SYS_BEGIN_CAN_SEND_SELF_SIGNALS(process_state);

    pid_t pid = (pid_t) process_state->cpu_state.rsi;
    int signum = (int) process_state->cpu_state.rdx;

    struct process *current = get_current_process();

    // pid -1 is not yet implemented
    if (signum < 1 || signum > _NSIG || pid == -1) {
        SYS_RETURN(-EINVAL);
    }

    if (pid == 0) {
        pid = -current->pgid;
    }

    if (pid < 0) {
        SYS_RETURN(signal_process_group(-pid, signum));
    } else {
        SYS_RETURN(signal_process(pid, signum));
    }
}

void arch_sys_setpgid(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    pid_t pid = (pid_t) process_state->cpu_state.rsi;
    pid_t pgid = (pid_t) process_state->cpu_state.rdx;

    if (pgid < 0) {
        SYS_RETURN(-EINVAL);
    }

    if (pid == 0) {
        pid = get_current_process()->pid;
    }

    if (pgid == 0) {
        pgid = get_current_process()->pid;
    }

#ifdef SET_PGID_DEBUG
    debug_log("Setting pgid: [ %d, %d, %d ]\n", pid, get_current_process()->pgid, pgid);
#endif /* SET_PGID_DEBUG */

    // FIXME: process needs a lock to prevent races
    struct process *process = find_by_pid(pid);
    if (process == NULL) {
        SYS_RETURN(-ESRCH);
    }

    process->pgid = pgid;

    proc_update_pgid(pid, pgid);
    SYS_RETURN(0);
}

void arch_sys_sigaction(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int signum = (int) process_state->cpu_state.rsi;
    const struct sigaction *act = (const struct sigaction*) process_state->cpu_state.rdx;
    struct sigaction *old_act = (struct sigaction*) process_state->cpu_state.rcx;

    if (signum <= 0 || signum > _NSIG) {
        SYS_RETURN(-EINVAL);
    }

    struct process *current = get_current_process();
    if (old_act != NULL) {
        memcpy(old_act, &current->sig_state[signum], sizeof(struct sigaction));
    }

    if (act != NULL) {
#ifdef SIGACTION_DEBUG
        debug_log("Changing signal state: [ %d, %#.16lX ]\n", signum, (uintptr_t) act->sa_handler);
#endif /* SIGACTION_DEBUG */
        memcpy(&current->sig_state[signum], act, sizeof(struct sigaction));
    }

    SYS_RETURN(0);
}

void arch_sys_sigreturn(struct process_state *process_state) {
    struct process *process = get_current_process();
    uint64_t *mask_ptr = (uint64_t*) process_state->stack_state.rsp;
    struct process_state *saved_state = (struct process_state*) (mask_ptr + 1);

    debug_log("Sig return\n");

    memcpy(&process->arch_process.process_state, saved_state, sizeof(struct process_state));

    // Restore mask
    process->sig_mask = *mask_ptr;

    yield_signal();
}

void arch_sys_sigprocmask(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int how = (int) process_state->cpu_state.rsi;
    const sigset_t *set = (const sigset_t*) process_state->cpu_state.rdx;
    sigset_t *old = (sigset_t*) process_state->cpu_state.rcx;

    struct process *current = get_current_process();

    if (old) {
        *old = current->sig_mask;
    }

    if (set) {
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
    }

    SYS_RETURN(0);
}

void arch_sys_dup(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int oldfd = (int) process_state->cpu_state.rsi;

    struct process *current = get_current_process();
    if (current->files[oldfd] == NULL) {
        SYS_RETURN(-EBADF);
    }

    // Should lock process to prevent races
    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (current->files[i] == NULL) {
            current->files[i] = fs_dup(current->files[oldfd]);
#ifdef DUP_DEBUG
            debug_log("Dup: [ %d, %lu ]\n", oldfd, i);
#endif /* DUP_DEBUG */
            SYS_RETURN(i);
        }
    }

    SYS_RETURN(-EMFILE);
}

void arch_sys_getpgid(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    pid_t pid = (pid_t) process_state->cpu_state.rsi;
    if (pid == 0) {
        pid = get_current_process()->pid;
    }

    SYS_RETURN(proc_get_pgid(pid));
}

void arch_sys_sleep(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    unsigned int seconds = (unsigned int) process_state->cpu_state.rsi;

    debug_log("Sleeping: [ %u ]\n", seconds);

    struct process *current = get_current_process();

    disable_interrupts();
    current->sleeping = true;
    current->sleep_end = get_time() + seconds * 1000;
    current->sched_state = WAITING;
    yield();

    current->sleeping = false;
    SYS_RETURN(seconds);
}

void arch_sys_access(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_path = (const char*) process_state->cpu_state.rsi;
    int mode = (int) process_state->cpu_state.rdx;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_access(path, mode);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_accept(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr*) process_state->cpu_state.rdx;
    socklen_t *addrlen = (socklen_t*) process_state->cpu_state.rcx;
    int flags = (int) process_state->cpu_state.r8;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_accept(file, addr, addrlen, flags));
}

void arch_sys_bind(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    const struct sockaddr *addr = (const struct sockaddr*) process_state->cpu_state.rdx;
    socklen_t addrlen = (socklen_t) process_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_bind(file, addr, addrlen));
}

void arch_sys_connect(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr*) process_state->cpu_state.rdx;
    socklen_t addrlen = (socklen_t) process_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_connect(file, addr, addrlen));
}

void arch_sys_listen(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    int backlog = (int) process_state->cpu_state.rdx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_listen(file, backlog));
}

void arch_sys_socket(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int domain = (int) process_state->cpu_state.rsi;
    int type = (int) process_state->cpu_state.rdx;
    int protocol = (int) process_state->cpu_state.rcx;

    SYS_RETURN(net_socket(domain, type, protocol));
}

void arch_sys_shutdown(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    int how = (int) process_state->cpu_state.rdx;

    (void) fd;
    (void) how;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_getsockopt(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    int level = (int) process_state->cpu_state.rdx;
    int optname = (int) process_state->cpu_state.rcx;
    const void *optval = (const void*) process_state->cpu_state.r8;
    socklen_t *optlen = (socklen_t*) process_state->cpu_state.r9;

    (void) fd;
    (void) level;
    (void) optname;
    (void) optval;
    (void) optlen;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_setsockopt(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    int level = (int) process_state->cpu_state.rdx;
    int optname = (int) process_state->cpu_state.rcx;
    const void *optval = (const void*) process_state->cpu_state.r8;
    socklen_t optlen = (socklen_t) process_state->cpu_state.r9;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_setsockopt(file, level, optname, optval, optlen));
}

void arch_sys_getpeername(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr*) process_state->cpu_state.rdx;
    socklen_t *addrlen = (socklen_t*) process_state->cpu_state.rcx;

    (void) fd;
    (void) addr;
    (void) addrlen;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_getsockname(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    struct sockaddr *addr = (struct sockaddr*) process_state->cpu_state.rdx;
    socklen_t *addrlen = (socklen_t*) process_state->cpu_state.rcx;

    (void) fd;
    (void) addr;
    (void) addrlen;

    SYS_RETURN(-ENOSYS);
}

void arch_sys_sendto(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    const void *buf = (const void*) process_state->cpu_state.rdx;
    size_t len = (size_t) process_state->cpu_state.rcx;
    int flags = (int) process_state->cpu_state.r8;
    const struct sockaddr *dest = (const struct sockaddr*) process_state->cpu_state.r9;
    socklen_t addrlen = (socklen_t) process_state->cpu_state.r10;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_sendto(file, buf, len, flags, dest, addrlen));
}

void arch_sys_recvfrom(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    void *buf = (void*) process_state->cpu_state.rdx;
    size_t len = (size_t) process_state->cpu_state.rcx;
    int flags = (int) process_state->cpu_state.r8;
    struct sockaddr *source = (struct sockaddr*) process_state->cpu_state.r9;
    socklen_t *addrlen = (socklen_t*) process_state->cpu_state.r10;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct file *file = get_current_process()->files[fd];
    if (!file) {
        SYS_RETURN(-EBADF);
    }

    if (!(file->flags & FS_SOCKET)) {
        SYS_RETURN(-ENOTSOCK);
    }

    SYS_RETURN(net_recvfrom(file, buf, len, flags, source, addrlen));
}

void arch_sys_mmap(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    void *addr = (void*) process_state->cpu_state.rsi;
    size_t length = (size_t) process_state->cpu_state.rdx;
    int prot = (int) process_state->cpu_state.rcx;
    int flags = (int) process_state->cpu_state.r8;
    int fd = (int) process_state->cpu_state.r9;
    off_t offset = (off_t) process_state->cpu_state.r10;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EINVAL);
    }

    struct file *file = get_current_process()->files[fd];

    SYS_RETURN(fs_mmap(addr, length, prot, flags, file, offset));
}

void arch_sys_munmap(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    void *addr = (void*) process_state->cpu_state.rsi;
    size_t length = (size_t) process_state->cpu_state.rdx;

    SYS_RETURN(fs_munmap(addr, length));
}

void arch_sys_rename(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    const char *_old_path = (const char*) process_state->cpu_state.rsi;
    const char *_new_path = (const char*) process_state->cpu_state.rdx;

    if (_old_path == NULL || _new_path == NULL) {
        SYS_RETURN(-EINVAL);
    }

    struct process *current = get_current_process();
    char *old_path = get_full_path(current->cwd, _old_path);
    char *new_path = get_full_path(current->cwd, _new_path);

    int ret = fs_rename(old_path, new_path);

    free(old_path);
    free(new_path);

    SYS_RETURN(ret);
}

void arch_sys_fcntl(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    int command = (int) process_state->cpu_state.rdx;
    int arg = (int) process_state->cpu_state.rcx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct process *current = get_current_process();
    struct file *file = current->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_fcntl(file, command, arg));
}

void arch_sys_fstat(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    struct stat *stat_struct = (struct stat*) process_state->cpu_state.rdx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct process *current = get_current_process();
    struct file *file = current->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_fstat(file, stat_struct));
}

void arch_sys_alarm(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    unsigned int seconds = (unsigned int) process_state->cpu_state.rsi;


    debug_log("Sleeping: [ %u ]\n", seconds);

    struct process *current = get_current_process();

    disable_interrupts();
    current->sleeping = true;
    current->sleep_end = get_time() + seconds * 1000;
    current->sched_state = WAITING;
    yield();

    current->sleeping = false;

    disable_interrupts();
    current->can_send_self_signals = true;
    signal_process(current->pid, SIGALRM);
    SYS_RETURN(0);
}

void arch_sys_fchmod(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    int fd = (int) process_state->cpu_state.rsi;
    mode_t mode = (mode_t) process_state->cpu_state.rdx;

    if (fd < 0 || fd > FOPEN_MAX) {
        SYS_RETURN(-EBADF);
    }

    struct process *current = get_current_process();
    struct file *file = current->files[fd];
    if (file == NULL) {
        SYS_RETURN(-EBADF);
    }

    SYS_RETURN(fs_fchmod(file, mode));
}

void arch_sys_getppid(struct process_state *process_state) {
    SYS_BEGIN(process_state);

    SYS_RETURN(get_current_process()->ppid);
}