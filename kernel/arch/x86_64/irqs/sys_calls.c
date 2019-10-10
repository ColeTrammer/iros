#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/elf64.h>
#include <kernel/sched/process_sched.h>
#include <kernel/fs/vfs.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/arch/x86_64/proc/process.h>
#include <kernel/hal/x86_64/gdt.h>

#define SYS_RETURN(val)                       \
    do {                                      \
        process_state->cpu_state.rax = (val); \
        return;                               \
    } while (0)

void arch_sys_exit(struct process_state *process_state) {
    /* Disable Interrups To Prevent Premature Process Removal, Since Sched State Is Set */
    disable_interrupts();

    struct process *process = get_current_process();
    process->sched_state = EXITING;

    int exit_code = (int) process_state->cpu_state.rsi;
    debug_log("Process Exited: [ %d, %d ]\n", process->pid, exit_code);

    sys_sched_run_next(process_state);
}

void arch_sys_sbrk(struct process_state *process_state) {
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
    struct process *parent = get_current_process();
    struct process *child = calloc(1, sizeof(struct process));
    child->pid = get_next_pid();
    child->sched_state = READY;
    child->kernel_process = false;
    child->process_memory = clone_process_vm();

    debug_log("Forking Process: [ %d ]\n", parent->pid);

    memcpy(&child->arch_process.process_state, process_state, sizeof(struct process_state));
    child->arch_process.process_state.cpu_state.rax = 0;
    child->arch_process.cr3 = clone_process_paging_structure();
    child->arch_process.kernel_stack = KERNEL_PROC_STACK_START;
    child->arch_process.setup_kernel_stack = true;
    child->cwd = malloc(strlen(parent->cwd) + 1);
    strcpy(child->cwd, parent->cwd);

    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (parent->files[i]) {
            child->files[i] = fs_clone(parent->files[i]);
        }
    }

    sched_add_process(child);

    SYS_RETURN(child->pid);
}

void arch_sys_open(struct process_state *process_state) {
    const char *_path = (const char*) process_state->cpu_state.rsi;
    int flags = (int) process_state->cpu_state.rdx;
    mode_t mode = (mode_t) process_state->cpu_state.rcx;
    
    assert(_path != NULL);

    int error = 0;

    struct process *process = get_current_process();
    char *path = get_full_path(process->cwd, _path);

    struct file *file = fs_open(path, &error);

    if (file == NULL) {
        if (flags & O_CREAT) {
            debug_log("Creating file: [ %s ]\n", path);

            error = fs_create(path, mode);
            if (error) {
                free(path);
                SYS_RETURN((uint64_t) error);
            }

            file = fs_open(path, &error);
            if (file == NULL) {
                free(path);
                SYS_RETURN((uint64_t) error);
            }
        } else {
            debug_log("File Not Found\n");
            free(path);
            SYS_RETURN((uint64_t) error);
        }
    }

    /* Should probably be some other error instead */
    if (!(file->flags & FS_DIR) && (flags & O_DIRECTORY)) {
        free(path);
        SYS_RETURN(-EINVAL);
    }

    /* Handle append mode */
    if (flags & O_APPEND) {
        fs_seek(file, 0, SEEK_END);
    }

    /* Start at 3 because 0,1,2 are reserved for stdin, stdio, and stderr */
    for (size_t i = 3; i < FOPEN_MAX; i++) {
        if (process->files[i] == NULL) {
            process->files[i] = file;
            free(path);
            SYS_RETURN(i);
        }
    }

    /* Max files allocated, should return some ERROR */
    free(path);
    assert(false);
}

void arch_sys_read(struct process_state *process_state)  {
    int fd = (int) process_state->cpu_state.rsi;
    char *buf = (void*) process_state->cpu_state.rdx;
    size_t count = (size_t) process_state->cpu_state.rcx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file != NULL);

    SYS_RETURN((uint64_t) fs_read(file, buf, count));
}

void arch_sys_write(struct process_state *process_state) {
    int fd = (int) process_state->cpu_state.rsi;
    void *buf = (void*) process_state->cpu_state.rdx;
    size_t count = (size_t) process_state->cpu_state.rcx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file != NULL);

    SYS_RETURN((uint64_t) fs_write(file, buf, count));
}

void arch_sys_close(struct process_state *process_state) {
    int fd = (int) process_state->cpu_state.rsi;

    struct process *process = get_current_process();
    assert(process->files[fd]);
    int error = fs_close(process->files[fd]);
    process->files[fd] = NULL;

    SYS_RETURN(error);
}

void arch_sys_execve(struct process_state *process_state) {
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
    struct file *program = fs_open(path, &error);
    if (program == NULL) {
        /* Should look at $PATH variable, instead is currently hardcoded */
        char *path_list[] = { "/initrd", "/bin", "/usr/bin", NULL };

        size_t i = 0;
        for (char *prefix = path_list[i]; prefix != NULL; prefix = path_list[++i]) {
            free(path);
            path = get_full_path(prefix, file_name);

            error = 0;
            program = fs_open(path, &error);

            if (program != NULL) {
                break;
            }
        }

        if (program == NULL) {
            free(path);
            SYS_RETURN((uint64_t) error);
        }
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

    fs_close(program);

    if (!elf64_is_valid(buffer)) {
        free(path);
        free(buffer);
        SYS_RETURN(-ENOEXEC);
    }

    struct process *process = calloc(1, sizeof(struct process));

    /* Clone open file descriptors (should close dirs and things marked with FD_CLOEXEC, but doesn't) */
    for (size_t i = 0; i < FOPEN_MAX; i++) {
        process->files[i] = fs_clone(current->files[i]);
    }

    /* Clone vm_regions so that they can be freed later */
    struct vm_region *__process_stack = get_vm_region(current->process_memory, VM_PROCESS_STACK);
    struct vm_region *__kernel_stack = get_vm_region(current->process_memory, VM_KERNEL_STACK);

    struct vm_region *process_stack = calloc(1, sizeof(struct vm_region));
    struct vm_region *kernel_stack = calloc(1, sizeof(struct vm_region));

    memcpy(process_stack, __process_stack, sizeof(struct vm_region));
    memcpy(kernel_stack, __kernel_stack, sizeof(struct vm_region));

    process->pid = current->pid;
    process->process_memory = kernel_stack;
    process->process_memory = add_vm_region(process->process_memory, process_stack);
    process->kernel_process = false;
    process->sched_state = READY;
    process->cwd = malloc(strlen(current->cwd) + 1);
    strcpy(process->cwd, current->cwd);
    process->next = NULL;

    process->arch_process.cr3 = get_cr3();
    process->arch_process.kernel_stack = KERNEL_PROC_STACK_START;

    struct virt_page_info *info = calloc(1, sizeof(struct virt_page_info));
    memcpy(info, current->arch_process.kernel_stack_info, sizeof(struct virt_page_info));

    process->arch_process.kernel_stack_info = info;
    process->arch_process.setup_kernel_stack = false;

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
    free_process(current, false, false);
    sched_add_process(process);

    sys_sched_run_next(&process->arch_process.process_state);
}

void arch_sys_waitpid(struct process_state *process_state) {
    pid_t pid = (pid_t) process_state->cpu_state.rsi;
    int *status = (int*) process_state->cpu_state.rdx;
    int flags = (int) process_state->cpu_state.rcx;

    assert(pid > 0);
    assert(status != NULL);
    assert(flags == WUNTRACED);

    /* Hack To Implement Waiting: Poll the process list until the process is removed, yielding until that happens */
    struct process *proc;
    for (;;) {
        proc = find_by_pid(pid);
        if (proc == NULL) {
            break;
        }

        yield();
    }

    /* Indicated Process Has Exited */
    *status = 0;

    /* Indicated Success */
    SYS_RETURN(0);
}

void arch_sys_getpid(struct process_state *process_state) {
    SYS_RETURN(get_current_process()->pid);
}

void arch_sys_getcwd(struct process_state *process_state) {
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

    process->cwd = get_tnode_path(tnode);
    debug_log("Chdir: [ %s ]\n", process->cwd);

    free(path);
    SYS_RETURN(0);
}

void arch_sys_stat(struct process_state *process_state) {
    const char *_path = (const char*) process_state->cpu_state.rsi;
    void *stat_struct = (void*) process_state->cpu_state.rdx;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_stat(path, stat_struct);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_lseek(struct process_state *process_state) {
    int fd = (int) process_state->cpu_state.rsi;
    off_t offset = (off_t) process_state->cpu_state.rdx;
    int whence = (int) process_state->cpu_state.rcx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file);

    SYS_RETURN((uint64_t) fs_seek(file, offset, whence));
}

void arch_sys_ioctl(struct process_state *process_state) {
    int fd = (int) process_state->cpu_state.rsi;
    unsigned long request = (unsigned long) process_state->cpu_state.rdx;
    void *argp = (void*) process_state->cpu_state.rcx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file);

    SYS_RETURN((uint64_t) fs_ioctl(file, request, argp));
}

void arch_sys_ftruncate(struct process_state *process_state) {
    int fd = (int) process_state->cpu_state.rsi;
    off_t length = (off_t) process_state->cpu_state.rdx;

    struct process *process = get_current_process();
    struct file *file = process->files[fd];
    assert(file);

    SYS_RETURN((uint64_t) fs_truncate(file, length));
}

void arch_sys_time(struct process_state *process_state) {
    SYS_RETURN((uint64_t) get_time());
}

void arch_sys_mkdir(struct process_state *process_state) {
    const char *pathname = (const char*) process_state->cpu_state.rsi;
    mode_t mode = (mode_t) process_state->cpu_state.rdx;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, pathname);

    int ret = fs_mkdir(path, mode);

    free(path);
    SYS_RETURN((uint64_t) ret);
}

void arch_sys_dup2(struct process_state *process_state) {
    int oldfd = (int) process_state->cpu_state.rsi;
    int newfd = (int) process_state->cpu_state.rdx;

    debug_log("Dup: [ %d, %d ]\n", oldfd, newfd);

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

    process->files[newfd] = fs_clone(process->files[oldfd]);
    SYS_RETURN(0);
}

void arch_sys_pipe(struct process_state *process_state) {
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
    const char *_path = (const char*) process_state->cpu_state.rsi;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_unlink(path);
    free(path);

    SYS_RETURN(ret);
}

void arch_sys_rmdir(struct process_state *process_state) {
    const char *_path = (const char*) process_state->cpu_state.rsi;

    struct process *current = get_current_process();
    char *path = get_full_path(current->cwd, _path);

    int ret = fs_rmdir(path);
    free(path);

    SYS_RETURN(ret);
}