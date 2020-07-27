#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/gdt.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>

#define STACK_TRACE_ON_ANY_SIGSEGV

#define SIZEOF_IRETQ_INSTRUCTION 2 // bytes

extern struct process initial_kernel_process;

static void kernel_idle() {
    for (;;)
        ;
}

pid_t proc_fork(void) {
    struct task *parent = get_current_task();
    struct task *child = calloc(1, sizeof(struct task));
    struct process *child_process = calloc(1, sizeof(struct process));
    child->process = child_process;

    child->tid = get_next_tid();
    child_process->pid = get_next_pid();
    child_process->main_tid = child->tid;
    init_mutex(&child_process->lock);
    init_spinlock(&child_process->user_mutex_lock);
    proc_add_process(child_process);
    child->sched_state = RUNNING_INTERRUPTIBLE;
    child->kernel_task = false;
    child_process->process_memory = clone_process_vm();
    child_process->tty = parent->process->tty;
    child_process->task_list = child;

    debug_log("Forking Task: [ %d ]\n", parent->process->pid);

    memcpy(&child->arch_task.task_state, parent->arch_task.user_task_state, sizeof(struct task_state));
    child->arch_task.task_state.cpu_state.rax = 0;
    child_process->arch_process.cr3 = create_clone_process_paging_structure(child_process);
    child->kernel_stack = vm_allocate_kernel_region(KERNEL_STACK_SIZE);
    child->arch_task.user_thread_pointer = parent->arch_task.user_thread_pointer;
    child_process->cwd = bump_tnode(parent->process->cwd);
    child_process->pgid = parent->process->pgid;
    child_process->ppid = parent->process->pid;
    child->process->uid = parent->process->uid;
    child->process->euid = parent->process->euid;
    child->process->gid = parent->process->gid;
    child->process->egid = parent->process->egid;
    child->process->sid = parent->process->sid;
    child->process->umask = parent->process->umask;
    child->process->start_time = time_read_clock(CLOCK_REALTIME);
    child->sig_pending = 0;
    child->sig_mask = parent->sig_mask;
    child_process->exe = bump_tnode(parent->process->exe);
    child_process->name = strdup(parent->process->name);
    memcpy(&child_process->sig_state, &parent->process->sig_state, sizeof(struct sigaction) * _NSIG);
    child_process->process_clock = time_create_clock(CLOCK_PROCESS_CPUTIME_ID);
    child->task_clock = time_create_clock(CLOCK_THREAD_CPUTIME_ID);

    child_process->supplemental_gids_size = parent->process->supplemental_gids_size;
    child_process->supplemental_gids = malloc(parent->process->supplemental_gids_size * sizeof(gid_t));
    memcpy(child_process->supplemental_gids, parent->process->supplemental_gids, parent->process->supplemental_gids_size * sizeof(gid_t));

    task_align_fpu(child);
    memcpy(child->fpu.aligned_state, parent->fpu.aligned_state, FPU_IMAGE_SIZE);

    for (size_t i = 0; i < FOPEN_MAX; i++) {
        if (parent->process->files[i].file) {
            child_process->files[i] = fs_dup_accross_fork(parent->process->files[i]);
        }
    }

    disable_interrupts();
    sched_add_task(child);
    return child_process->pid;
}

static int execve_helper(char **path, char *buffer, size_t buffer_length, struct file **file, char ***prepend_argv,
                         size_t *prepend_argv_length, int *depth, char **argv) {
    if (*file) {
        fs_close(*file);
    }

    int ret = 0;
    *file = fs_openat(get_current_task()->process->cwd, *path, O_RDONLY, 0, &ret);
    if (ret < 0) {
        return ret;
    }

    ssize_t nread = fs_read(*file, buffer, buffer_length);
    if (nread < 0) {
        fs_close(*file);
        return (int) nread;
    } else if (nread == 0) {
        fs_close(*file);
        return -ENOEXEC;
    }

    if (!elf64_is_valid(buffer)) {
        if (memcmp(buffer, "#!", 2) == 0) {
            debug_log("Encoutered #!\n");
            bool first = *prepend_argv_length == 0;

            char *path_save = NULL;
            if (first) {
                path_save = strdup(*path);
            }
            size_t path_len = strcspn(buffer + 2, " \n");
            char restore = buffer[2 + path_len];
            buffer[2 + path_len] = '\0';
            free(*path);
            *path = strdup(buffer + 2);
            debug_log("#!: [ %s ]\n", *path);
            buffer[2 + path_len] = restore;
            bool has_extra_arg = false;
            size_t extra_arg_start = 0;

            size_t i;
            for (i = 0; buffer[2 + path_len + i] != '\n' && buffer[2 + path_len + i] != '\0'; i++) {
                if (!extra_arg_start && !isspace(buffer[2 + path_len + i])) {
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
                buffer[2 + path_len] = '\0';
                *prepend_argv[*prepend_argv_length - 3] = strdup(buffer + 2);
                buffer[2 + path_len] = restore;
                buffer[2 + path_len + i] = '\0';
                *prepend_argv[*prepend_argv_length - 2] = strdup(buffer + extra_arg_start);
            } else {
                buffer[2 + path_len] = '\0';
                *prepend_argv[*prepend_argv_length - 2] = strdup(buffer + 2);
                buffer[2 + path_len] = restore;
            }

            (*prepend_argv)[*prepend_argv_length - 1] = NULL;

            if (first) {
                argv[0] = path_save;
            }

            (*depth)++;
            return execve_helper(path, buffer, buffer_length, file, prepend_argv, prepend_argv_length, depth, argv);
        }

        fs_close(*file);
        return -ENOEXEC;
    }

    return 0;
}

int proc_execve(char *path, char **argv, char **envp) {
    struct task *current = get_current_task();

    debug_log("Exec Task: [ %d, %s ]\n", current->process->pid, path);

    path = strdup(path);
    char temp_buffer[512];
    size_t temp_buffer_length = sizeof(temp_buffer);
    struct file *file = NULL;
    char **prepend_argv = NULL;
    size_t prepend_argv_length = 0;
    int depth = 0;
    int error = execve_helper(&path, temp_buffer, temp_buffer_length, &file, &prepend_argv, &prepend_argv_length, &depth, argv);
    free(path);
    if (error) {
        for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
            free(prepend_argv[i]);
        }
        free(prepend_argv);
        return error;
    }

    struct task *task = calloc(1, sizeof(struct task));
    struct process *process = calloc(1, sizeof(struct process));
    task->process = process;

    assert(file->tnode);
    struct tnode *tnode = bump_tnode(file->tnode);
    struct inode *inode = fs_file_inode(file);
    process->exe = tnode;
    process->name = prepend_argv != NULL ? argv[0] : strdup(argv[0]);

    process->uid = current->process->uid;
    if (depth == 0 && (inode->mode & S_ISUID)) {
        process->euid = inode->uid;
    } else {
        process->euid = current->process->euid;
    }

    process->gid = current->process->gid;
    if (depth == 0 && (inode->mode & S_ISGID)) {
        process->egid = inode->gid;
    } else {
        process->egid = current->process->egid;
    }

    // Dup open file descriptors
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (!current->process->files[i].file ||
            ((current->process->files[i].fd_flags & FD_CLOEXEC) || (current->process->files[i].file->flags & FS_DIR))) {
            // NOTE: the files will be closed by the `free_task` function
            continue;
        }

        process->files[i] = fs_dup(current->process->files[i]);
    }

    process->supplemental_gids_size = current->process->supplemental_gids_size;
    process->supplemental_gids = malloc(current->process->supplemental_gids_size * sizeof(gid_t));
    memcpy(process->supplemental_gids, current->process->supplemental_gids, current->process->supplemental_gids_size * sizeof(gid_t));

    task->tid = current->tid;
    process->pid = current->process->pid;
    process->main_tid = task->tid;
    process->task_list = task;
    init_mutex(&process->lock);
    init_spinlock(&process->user_mutex_lock);
    process->pgid = current->process->pgid;
    process->ppid = current->process->ppid;
    process->sid = current->process->sid;
    process->umask = current->process->umask;
    process->start_time = time_read_clock(CLOCK_REALTIME);

    task->kernel_task = false;
    process->tty = current->process->tty;
    process->cwd = bump_tnode(current->process->cwd);
    task->sig_mask = current->sig_mask;
    memcpy(&process->times, &current->process->times, sizeof(struct tms));
    process->process_clock = time_create_clock(CLOCK_PROCESS_CPUTIME_ID);
    task->task_clock = time_create_clock(CLOCK_THREAD_CPUTIME_ID);

    // Clone only signal dispositions that don't have a handler
    for (int i = 0; i < _NSIG; i++) {
        if ((uintptr_t) current->process->sig_state[i].sa_handler <= (uintptr_t) SIG_IGN) {
            memcpy(&process->sig_state[i], &current->process->sig_state[i], sizeof(struct sigaction));
        }
    }

    process->arch_process.cr3 = get_cr3();

    task_align_fpu(task);
    memcpy(task->fpu.aligned_state, get_idle_task()->fpu.aligned_state, FPU_IMAGE_SIZE);

    task->arch_task.user_task_state = current->arch_task.user_task_state;
    task->arch_task.user_task_state->stack_state.cs = USER_CODE_SELECTOR;
    task->arch_task.user_task_state->stack_state.rflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    task->arch_task.user_task_state->stack_state.ss = USER_DATA_SELECTOR;

    size_t length = fs_file_size(file);
    proc_clone_program_args(process, prepend_argv, argv, envp);

    /* Ensure File Name And Args Are Still Mapped */
    soft_remove_paging_structure(current->process->process_memory);

    /* Disable Preemption So That Nothing Goes Wrong When Removing Ourselves (We Don't Want To Remove Ourselves From The List And Then Be
     * Interrupted) */
    uint64_t save = disable_interrupts_save();

    // Tranfer ownership of the current's kernel stack to avoid the need for additional allocation.
    task->kernel_stack = current->kernel_stack;
    current->kernel_stack = NULL;

    local_sched_remove_task(current);
    local_sched_add_task(task);

    set_current_task(task);
    task->in_kernel = true;
    task->sched_state = RUNNING_UNINTERRUPTIBLE;

    // NOTE: once we re-enable interrupts we're effectively running as the new task inside a system call.
    interrupts_restore(save);

    // Free the old task's resources now that we can block.
    free_task(current, false);

    // FIXME: adding the process now might be a bit prone to race conditions
    proc_add_process(process);

    char *buffer = (char *) fs_mmap(NULL, length, PROT_READ, MAP_SHARED, file, 0);
    // FIXME: this assert is very dangerous, but we can't return an error condition since
    //        we've already destroyed the old process's address space.
    assert(buffer != MAP_FAILED);

    struct initial_process_info info = { 0 };
    info.main_tid = task->tid;
    info.isatty_mask = 0;
    for (int i = 0; i <= 3; i++) {
        struct file *file = task->process->files[i].file;
        if (file && !fs_ioctl(file, TISATTY, NULL)) {
            info.isatty_mask |= (1 << i);
        }
    }

    assert(elf64_is_valid(buffer));
    task->arch_task.user_task_state->stack_state.rip = elf64_load_program(buffer, length, file, &info);
    elf64_map_heap(buffer, task);

    fs_close(file);
    unmap_range((uintptr_t) buffer, length);

    uintptr_t stack_end = proc_allocate_user_stack(process, &info);
    task->arch_task.user_task_state->stack_state.rsp = map_program_args(stack_end, process->args_context, &info, task);

    for (size_t i = 0; prepend_argv && i < prepend_argv_length; i++) {
        free(prepend_argv[i]);
    }
    free(prepend_argv);

    return 0;
}

static void load_task_into_memory(struct task *task) {
    if (task->kernel_stack) {
        set_tss_stack_pointer(task->kernel_stack->end);
    }

    if (task->process->arch_process.cr3 != get_cr3()) {
        load_cr3(task->process->arch_process.cr3);
    }

    if (!task->kernel_task) {
        fxrstor(task->fpu.aligned_state);
        set_msr(MSR_FS_BASE, (uint64_t) task->arch_task.user_thread_pointer);
    }
}

void task_align_fpu(struct task *task) {
    uintptr_t unaligned_fpu = (uintptr_t) &task->fpu.raw_fpu_state;
    task->fpu.aligned_state = (uint8_t *) ((unaligned_fpu & ~0xFULL) + 16ULL);
    assert(((uintptr_t) task->fpu.aligned_state) % 16 == 0);
    assert((uintptr_t) task->fpu.aligned_state >= unaligned_fpu &&
           (uintptr_t) task->fpu.aligned_state <= (uintptr_t) task->fpu.raw_fpu_state.image);
}

void arch_init_idle_task(struct task *idle_task, struct processor *processor) {
    idle_task->arch_task.task_state.stack_state.rip = (uint64_t) &kernel_idle;
    idle_task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    idle_task->arch_task.task_state.stack_state.rflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    idle_task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    idle_task->arch_task.task_state.stack_state.rsp = processor->kernel_stack->end;

    task_align_fpu(idle_task);
    fninit();
    fxsave(idle_task->fpu.aligned_state);
}

void arch_load_kernel_task(struct task *task, uintptr_t entry) {
    task->process->arch_process.cr3 = initial_kernel_process.arch_process.cr3;
    task->arch_task.task_state.cpu_state.rbp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.rip = entry;
    task->arch_task.task_state.stack_state.cs = CS_SELECTOR;
    task->arch_task.task_state.stack_state.rflags = get_rflags() | INTERRUPTS_ENABLED_FLAG;
    task->arch_task.task_state.stack_state.rsp = task->kernel_stack->end;
    task->arch_task.task_state.stack_state.ss = DATA_SELECTOR;
    task->in_kernel = true;
}

void arch_setup_program_args(struct task *task, struct initial_process_info *info, size_t argc, char **argv, char **envp) {
    task->arch_task.user_task_state->cpu_state.rdi = (uint64_t) info;
    task->arch_task.user_task_state->cpu_state.rsi = (uint64_t) argc;
    task->arch_task.user_task_state->cpu_state.rdx = (uint64_t) argv;
    task->arch_task.user_task_state->cpu_state.rcx = (uint64_t) envp;
}

/* Must be called from unpremptable context */
void arch_run_task(struct task *task) {
    load_task_into_memory(task);
    set_current_task(task);
    __run_task(&task->arch_task);
}

/* Must be called from unpremptable context */
void arch_free_task(struct task *task, bool free_paging_structure) {
    (void) task;
    (void) free_paging_structure;
}

void task_interrupt_blocking(struct task *task, int ret) {
    assert(task->blocking);
    task->arch_task.task_state.cpu_state.rax = (uint64_t) ret;
    task->blocking = false;
    task->sched_state = RUNNING_UNINTERRUPTIBLE;
}

bool proc_in_kernel(struct task *task) {
    return task->in_kernel;
}

void task_do_sig_handler(struct task *task, int signum) {
    assert(task->process->sig_state[signum].sa_handler != SIG_IGN);
    assert(task->process->sig_state[signum].sa_handler != SIG_DFL);

    if (task->blocking) {
        // Defer running the signal handler until after the blocking code
        // has a chance to clean up.
        task_interrupt_blocking(task, -EINTR);
        return;
    }

    // For debugging purposes (bash will catch SIGSEGV and try to cleanup)
#ifdef STACK_TRACE_ON_ANY_SIGSEGV
    if (signum == SIGSEGV) {
        elf64_stack_trace(task, true);
    }
#endif /* STACK_TRACE_ON_ANY_SIGSEGV */

    struct sigaction *act = &task->process->sig_state[signum];

    load_task_into_memory(task);

    uint64_t save_rsp =
        proc_in_kernel(task) ? task->arch_task.user_task_state->stack_state.rsp : task->arch_task.task_state.stack_state.rsp;
    if ((act->sa_flags & SA_ONSTACK) && (task->process->alt_stack.ss_flags & __SS_ENABLED)) {
        if (!(save_rsp >= (uintptr_t) task->process->alt_stack.ss_sp &&
              save_rsp <= (uintptr_t) task->process->alt_stack.ss_sp + task->process->alt_stack.ss_size)) {
            save_rsp = (((uintptr_t) task->process->alt_stack.ss_sp + task->process->alt_stack.ss_size)) & ~0xF;
        }
    }

    assert(save_rsp != 0);
    uint8_t *fpu_save_state = (uint8_t *) ((save_rsp - 128) & ~0xF) - FPU_IMAGE_SIZE; // Sub 128 to enforce red-zone
    ucontext_t *save_state = ((ucontext_t *) fpu_save_state) - 1;
    siginfo_t *info = ((siginfo_t *) save_state) - 1;
    if (act->sa_flags & SA_SIGINFO) {
        if (task->queued_signals->info.si_signo == signum) {
            memcpy(info, &task->queued_signals->info, sizeof(siginfo_t));
            task_dequeue_signal(task);
        } else {
            // NOTE: this probably shouldn't happen, but for now means that the
            //       signal was sent by kill instead of sigqueue.
            //       We probably should allocate a struct in this case
            info->si_code = SI_USER;
            task_unset_sig_pending(task, signum);
        }
    } else {
        task_unset_sig_pending(task, signum);
    }

    struct task_state *to_copy = proc_in_kernel(task) ? task->arch_task.user_task_state : &task->arch_task.task_state;

    uint64_t *stack_frame = ((uint64_t *) info) - 2;
    *stack_frame = (uintptr_t) act->sa_restorer;
    assert((uintptr_t) stack_frame % 16 == 0);

    save_state->uc_link = save_state;
    save_state->uc_sigmask = (uint64_t)(task->in_sigsuspend ? task->saved_sig_mask : task->sig_mask);

    if ((act->sa_flags & SA_ONSTACK) && (task->process->alt_stack.ss_flags & __SS_ENABLED)) {
        save_state->uc_stack.ss_flags = SS_ONSTACK;
        save_state->uc_stack.ss_size = task->process->alt_stack.ss_size;
        save_state->uc_stack.ss_sp = task->process->alt_stack.ss_sp;
    } else {
        save_state->uc_stack.ss_flags = SS_DISABLE;
        save_state->uc_stack.ss_size = 0;
        save_state->uc_stack.ss_sp = 0;
    }

    memcpy(&save_state->uc_mcontext, to_copy, sizeof(struct task_state));
    memcpy(fpu_save_state, task->fpu.aligned_state, FPU_IMAGE_SIZE);
    if (proc_in_kernel(task)) {
        if (task->in_sigsuspend) {
            task->in_sigsuspend = false;
            task->in_kernel = false;
        } else if ((act->sa_flags & SA_RESTART) && (task->arch_task.user_task_state->cpu_state.rax == (uint64_t) -EINTR)) {
            // Decrement %rip by the sizeof of the iretq instruction so that
            // the program will automatically execute int $0x80, restarting
            // the sys call in the easy way possible
            save_state->uc_mcontext.__stack_state.rip -= SIZEOF_IRETQ_INSTRUCTION;
        }
    }

    task->arch_task.task_state.cpu_state.rdi = signum;
    task->arch_task.task_state.cpu_state.rsi = (uintptr_t) info;
    task->arch_task.task_state.cpu_state.rdx = (uintptr_t) save_state;

    task->arch_task.task_state.stack_state.rip =
        ((act->sa_flags & SA_SIGINFO) ? (uintptr_t) act->sa_sigaction : (uintptr_t) act->sa_handler);
    task->arch_task.task_state.stack_state.rsp = (uintptr_t) stack_frame;
    task->arch_task.task_state.stack_state.ss = USER_DATA_SELECTOR;
    task->arch_task.task_state.stack_state.cs = USER_CODE_SELECTOR;

    task->sig_mask = act->sa_mask;
    if (!(act->sa_flags & SA_NODEFER)) {
        task->sig_mask |= (1U << (signum - 1));
    }

    if (act->sa_flags & SA_RESETHAND) {
        act->sa_handler = SIG_DFL;
    }

    debug_log("Running pid: [ %p, %#.16lX, %d ]\n", save_state, task->arch_task.task_state.stack_state.rip, signum);

    struct task *current = get_current_task();
    assert(current == task);
    current->sched_state = RUNNING_INTERRUPTIBLE;
    __run_task(&current->arch_task);
}
