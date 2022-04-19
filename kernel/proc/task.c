#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include <kernel/fs/procfs.h>
#include <kernel/fs/vfs.h>
#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/proc/task_finalizer.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/sched/task_sched.h>
#include <kernel/time/clock.h>

// #define START_DEBUG
// #define TASK_DEBUG
// #define TASK_SCHED_STATE_DEBUG
// #define TASK_SIGNAL_DEBUG

struct task initial_kernel_task;
struct process initial_kernel_process;
struct process idle_kernel_process = {
    .task_list = INIT_LIST(idle_kernel_process.task_list),
    .main_tid = 1,
};

enum sig_default_behavior {
    TERMINATE,
    TERMINATE_AND_DUMP,
    IGNORE,
    STOP,
    INVAL,
};

static enum sig_default_behavior sig_defaults[_NSIG] = {
    INVAL,              // INVAL
    TERMINATE,          // SIGHUP
    TERMINATE,          // SIGINT
    TERMINATE_AND_DUMP, // SIGQUIT
    TERMINATE_AND_DUMP, // SIGBUS
    TERMINATE_AND_DUMP, // SIGTRAP
    TERMINATE_AND_DUMP, // SIGABRT
    IGNORE,             // SIGCONT
    TERMINATE_AND_DUMP, // SIGFPE
    TERMINATE,          // SIGKILL
    STOP,               // SIGTTIN
    STOP,               // SIGTTOU
    TERMINATE,          // SIGILL
    TERMINATE,          // SIGPIPE
    TERMINATE,          // SIGALRM
    TERMINATE,          // SIGTERM
    TERMINATE_AND_DUMP, // SIGSEGV
    STOP,               // SIGSTOP
    STOP,               // SIGTSTP
    TERMINATE,          // SIGUSR1
    TERMINATE,          // SIGUSR2
    TERMINATE,          // SIGPOLL
    TERMINATE,          // SIGPROF
    TERMINATE_AND_DUMP, // SIGSYS
    IGNORE,             // SIGURG
    TERMINATE,          // SIGVTALRM
    TERMINATE_AND_DUMP, // SIGXCPU
    TERMINATE_AND_DUMP, // SIGXFSZ
    IGNORE,             // SIGCHLD
    IGNORE,             // SIGWINCH
};

void proc_clone_program_args(struct process *process, char **prepend_argv, char **argv, char **envp) {
    struct args_context *context = process->args_context = malloc(sizeof(struct args_context));

    context->prepend_argc = 0;
    size_t prepend_args_str_length = 0;
    while (prepend_argv && prepend_argv[context->prepend_argc] != NULL) {
        prepend_args_str_length += strlen(prepend_argv[context->prepend_argc++]) + 1;
    }

    context->argc = 0;
    size_t args_str_length = 0;
    while (argv[context->argc++] != NULL) {
        args_str_length += strlen(argv[context->argc - 1]) + 1;
    }

    context->envc = 0;
    size_t env_str_length = 0;
    while (envp[context->envc++] != NULL) {
        env_str_length += strlen(envp[context->envc - 1]) + 1;
    }

    context->prepend_args_copy = calloc(context->prepend_argc, sizeof(char **));
    context->args_copy = calloc(context->argc, sizeof(char **));
    context->envp_copy = calloc(context->envc, sizeof(char **));

    context->prepend_args_buffer = malloc(prepend_args_str_length + args_str_length);
    context->args_buffer = context->prepend_args_buffer + prepend_args_str_length;
    context->env_buffer = malloc(env_str_length);

    context->args_bytes = prepend_args_str_length + args_str_length;
    context->env_bytes = env_str_length;

    ssize_t j = 0;
    ssize_t i = 0;
    while (prepend_argv && prepend_argv[i]) {
        ssize_t last = j;
        while (prepend_argv[i][j - last] != '\0') {
            context->prepend_args_buffer[j] = prepend_argv[i][j - last];
            j++;
        }
        context->prepend_args_buffer[j++] = '\0';
        context->prepend_args_copy[i++] = context->prepend_args_buffer + last;
    }

    i = 0;
    j = 0;
    while (argv[i] != NULL) {
        ssize_t last = j;
        while (argv[i][j - last] != '\0') {
            context->args_buffer[j] = argv[i][j - last];
            j++;
        }
        context->args_buffer[j++] = '\0';
        context->args_copy[i++] = context->args_buffer + last;
    }
    context->args_copy[i] = NULL;

    j = 0;
    i = 0;
    while (envp[i] != NULL) {
        ssize_t last = j;
        while (envp[i][j - last] != '\0') {
            context->env_buffer[j] = envp[i][j - last];
            j++;
        }
        context->env_buffer[j++] = '\0';
        context->envp_copy[i++] = context->env_buffer + last;
    }
    context->envp_copy[i] = NULL;
}

/* Copying args and envp is necessary because they could be saved on the program stack we are about to overwrite */
uintptr_t map_program_args(uintptr_t start, struct args_context *context, struct initial_process_info *info, struct task *task) {
    struct initial_process_info *info_location = (struct initial_process_info *) (start - sizeof(struct initial_process_info));
    *info_location = *info;

    char **argv_start = (char **) (((uintptr_t) info_location) - sizeof(char **));

    size_t count = context->prepend_argc + context->argc + context->envc;
    char *args_start = (char *) (argv_start - count);

    ssize_t i;
    for (i = 0; context->args_copy[i] != NULL; i++) {
        args_start -= strlen(context->args_copy[i]) + 1;
        strcpy(args_start, context->args_copy[i]);
        argv_start[i - context->argc] = args_start;
    }

    for (i = 0; i < (ssize_t) context->prepend_argc; i++) {
        args_start -= strlen(context->prepend_args_copy[i]) + 1;
        strcpy(args_start, context->prepend_args_copy[i]);
        argv_start[i - context->prepend_argc - context->argc] = args_start;
    }

    argv_start[0] = NULL;

    for (i = 0; context->envp_copy[i] != NULL; i++) {
        args_start -= strlen(context->envp_copy[i]) + 1;
        strcpy(args_start, context->envp_copy[i]);
        argv_start[i - count] = args_start;
    }

    argv_start[-(context->prepend_argc + context->argc + 1)] = NULL;

    args_start = arch_setup_program_args(task, args_start, info_location, context->prepend_argc + context->argc - 1,
                                         argv_start - context->prepend_argc - context->argc, argv_start - count);
    return (uintptr_t) args_start;
}

void free_program_args(struct args_context *context) {
    free(context->prepend_args_copy);
    free(context->args_copy);
    free(context->envp_copy);
    free(context->prepend_args_buffer);
    free(context->env_buffer);
    free(context);
}

static spinlock_t tid_lock;
static int tid_counter = 2;

int get_next_tid() {
    spin_lock(&tid_lock);

    int tid = tid_counter++;

    spin_unlock(&tid_lock);
    return tid;
}

void init_kernel_process(void) {
    initial_kernel_process.pid = 1;
    init_mutex(&initial_kernel_process.lock);
    init_spinlock(&initial_kernel_process.user_mutex_lock);
    init_spinlock(&initial_kernel_process.children_lock);
    init_spinlock(&initial_kernel_process.parent_lock);
    init_wait_queue(&initial_kernel_process.one_task_left_queue);
    init_wait_queue(&initial_kernel_process.child_wait_queue);
    init_list(&initial_kernel_process.task_list);
    init_list(&initial_kernel_process.timer_list);
    initial_kernel_process.main_tid = 1;
    initial_kernel_process.pgid = 1;
    initial_kernel_process.tty = -1;
    initial_kernel_process.priority = PROCESS_DEFAULT_PRIORITY;
    initial_kernel_process.start_time = time_read_clock(CLOCK_REALTIME);

    initial_kernel_task.process = &initial_kernel_process;
    init_list(&initial_kernel_task.queued_signals);
    init_spinlock(&initial_kernel_task.sig_lock);
    init_spinlock(&initial_kernel_task.unblock_lock);
    list_append(&initial_kernel_process.task_list, &initial_kernel_task.process_list);
    initial_kernel_task.kernel_task = true;
    initial_kernel_task.sched_state = RUNNING_UNINTERRUPTIBLE;
    initial_kernel_task.process->tty = -1;
    initial_kernel_task.tid = 1;
    initial_kernel_task.sched_ticks_remaining = 5;

    arch_load_kernel_task(&initial_kernel_task, 0);
}

void init_idle_task(struct processor *processor) {
    struct task *task = calloc(1, sizeof(struct task));
    task->process = &idle_kernel_process;

    // NOTE: this would need locks if APs weren't initialized one at a time.
    list_append(&idle_kernel_process.task_list, &task->process_list);

    task->tid = get_next_tid();
    task->kernel_task = true;
    task->sched_state = RUNNING_UNINTERRUPTIBLE;
    init_list(&task->queued_signals);
    init_spinlock(&task->sig_lock);
    init_spinlock(&task->unblock_lock);

    arch_init_idle_task(task, processor);

    if (!get_current_task()) {
        set_current_task(task);
    }

    processor->idle_task = task;
}

struct task *load_kernel_task(uintptr_t entry, const char *name) {
    struct task *task = calloc(1, sizeof(struct task));
    struct process *process = calloc(1, sizeof(struct process));
    task->process = process;

    task->process->pid = get_next_pid();
    init_mutex(&process->lock);
    init_spinlock(&process->user_mutex_lock);
    init_spinlock(&process->children_lock);
    init_spinlock(&process->parent_lock);
    init_wait_queue(&process->one_task_left_queue);
    init_wait_queue(&process->child_wait_queue);
    init_list(&process->task_list);
    init_list(&process->timer_list);
    init_list(&task->queued_signals);
    init_spinlock(&task->sig_lock);
    init_spinlock(&task->unblock_lock);
    proc_add_process(process);
    task->process->pgid = task->process->pid;
    task->process->process_memory = NULL;
    list_append(&process->task_list, &task->process_list);
    task->kernel_task = true;
    task->sched_state = RUNNING_UNINTERRUPTIBLE;
    task->kernel_stack = vm_allocate_kernel_region(KERNEL_STACK_SIZE);
    task->process->tty = -1;
    task->tid = get_next_tid();
    task->process->cwd = bump_tnode(fs_root());
    task->process->main_tid = task->tid;
    process->name = strdup(name);
    process->start_time = time_read_clock(CLOCK_REALTIME);
    process->parent = &initial_kernel_process;
    process->priority = PROCESS_DEFAULT_PRIORITY;
    proc_bump_process(process);
    proc_add_child(&initial_kernel_process, process);

    arch_load_kernel_task(task, entry);

    debug_log("Loaded Kernel Task: [ %s ]\n", process->name);
    return task;
}

static void task_switch_from_kernel_to_user_mode(struct task *current) {
    uint64_t save = disable_interrupts_save();
    if (!current->process->cwd) {
        current->process->cwd = bump_tnode(fs_root());
    }
    current->in_kernel = true;
    current->kernel_task = false;

    arch_task_switch_from_kernel_to_user_mode(current);

    current->task_clock = time_create_clock(CLOCK_THREAD_CPUTIME_ID);
    current->process->process_clock = time_create_clock(CLOCK_PROCESS_CPUTIME_ID);
    current->process->umask = 0022;
    current->process->limits[RLIMIT_CORE] = (struct rlimit) { .rlim_cur = 0, .rlim_max = 0 };
    current->process->limits[RLIMIT_CPU] = (struct rlimit) { .rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY };
    current->process->limits[RLIMIT_DATA] = (struct rlimit) { .rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY };
    current->process->limits[RLIMIT_FSIZE] = (struct rlimit) { .rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY };
    current->process->limits[RLIMIT_NOFILE] = (struct rlimit) { .rlim_cur = FOPEN_MAX, .rlim_max = FOPEN_MAX };
    current->process->limits[RLIMIT_STACK] = (struct rlimit) { .rlim_cur = 2 * 1024 * 1024, .rlim_max = 2 * 1024 * 1024 };
    current->process->limits[RLIMIT_AS] = (struct rlimit) { .rlim_cur = RLIM_INFINITY, .rlim_max = RLIM_INFINITY };
    interrupts_restore(save);
}

void start_userland(void) {
    char *argv[3] = { argv[0] = "/bin/start", kernel_use_graphics() ? "-g" : "-v", NULL };
    char *envp[1] = { NULL };

    struct task_state task_state_save = { 0 };
    task_setup_user_state(&task_state_save);

    struct task *current = get_current_task();
    current->user_task_state = &task_state_save;

    task_switch_from_kernel_to_user_mode(current);

#ifdef START_DEBUG
    int error = 0;
    current->process->files[0] =
        (struct file_descriptor) { .file = fs_openat(fs_root(), "/dev/serial0", O_RDWR, 0, &error), .fd_flags = 0 };
    current->process->files[1] = fs_dup(current->process->files[0]);
    current->process->files[2] = fs_dup(current->process->files[0]);
#endif /* START_DEBUG */

    int ret = proc_execve("/bin/start", argv, envp);
    if (ret) {
        debug_log("Failed to exec /bin/start: [ %s ]\n", strerror(-ret));
        abort();
    }

    disable_interrupts();
    memcpy(&current->arch_task.task_state, &task_state_save, sizeof(struct task_state));
    run_task(current);
}

void init_userland(void) {
    struct task *init = load_kernel_task((uintptr_t) start_userland, "init");
    sched_add_task(init);
}

void task_yield_if_state_changed(struct task *task) {
    if (task->should_exit) {
#ifdef TASK_SCHED_STATE_DEBUG
        debug_log("setting sched state to EXITING: [ %d:%d ]\n", task->process->pid, task->tid);
#endif /* TASK_SCHED_STATE_DEBUG */
        task_exit(task);
        disable_interrupts();
        sched_run_next();
    }

    if (task->should_stop) {
        // Restart the system call so that when the task is resumed, it will begin gracefully.
        arch_task_prepare_to_restart_sys_call(task);

        task_stop(task);
        kernel_yield();
    }
}

int proc_waitpid(pid_t waitspec, int *status, int flags) {
    if (flags & ~(WUNTRACED | WNOHANG | WCONTINUED)) {
        return -EINVAL;
    }

    struct process *parent = get_current_process();
    struct process *child = NULL;
    pid_t pid;
    spin_lock(&parent->children_lock);
    for (;;) {
        int ret = wait_for_with_spinlock_interruptible(
            get_current_task(), (pid = proc_get_waitable_process(parent, waitspec, &child)) != 0 || !!(flags & WNOHANG),
            &parent->child_wait_queue, &parent->children_lock);
        if (ret) {
            return ret;
        }

        if (child) {
            enum process_state state = child->state;
            int info = child->exit_code;
            bool terminated_bc_signal = child->terminated_bc_signal;

            proc_consume_wait_info(parent, child, state);
            switch (state) {
                case PS_CONTINUED:
                    if (!(flags & WCONTINUED)) {
                        continue;
                    }
                    break;
                case PS_STOPPED:
                    if (!(flags & WUNTRACED)) {
                        continue;
                    }
                    break;
                case PS_TERMINATED:
                    break;
                default:
                    assert(false);
                    break;
            }

            if (status) {
                switch (state) {
                    case PS_CONTINUED:
                        *status = 0xFFFF;
                        break;
                    case PS_STOPPED:
                        *status = 0x80 | (info << 8);
                        break;
                    case PS_TERMINATED:
                        if (terminated_bc_signal) {
                            *status = info;
                        } else {
                            *status = info << 8;
                        }
                        break;
                    default:
                        assert(false);
                }
            }
        }
        spin_unlock(&parent->children_lock);
        return pid;
    }
}

pid_t proc_getppid(struct process *process) {
    // Pid 1 has no parent.
    if (process->pid == 1) {
        return 0;
    }

    struct process *parent = proc_get_parent(process);
    pid_t ppid = parent->pid;
    proc_drop_parent(parent);
    return ppid;
}

/* Must be called from unpremptable context */
void run_task(struct task *task) {
    arch_run_task(task);
}

void free_task(struct task *task, bool free_paging_structure) {
#ifdef TASK_DEBUG
    debug_log("destroying: [ %d:%d ]\n", task->process->pid, task->tid);
#endif /* TASK_DEBUG */

    arch_free_task(task, free_paging_structure);

    list_for_each_entry_safe(&task->queued_signals, queued_signal, struct queued_signal, queue) {
        list_remove(&queued_signal->queue);
        task_free_queued_signal(queued_signal);
    }

    if (task->kernel_stack) {
        vm_free_kernel_region(task->kernel_stack);
    }

    if (task->task_clock) {
        time_destroy_clock(task->task_clock);
    }

    proc_drop_process(task->process, task, free_paging_structure);
    free(task);
}

bool task_unblock(struct task *task, int result) {
#ifdef TASK_DEBUG
    debug_log("unblocking task: [ %d:%d ]\n", task->tid, task->process->pid);
#endif /* TASK_DEBUG */

    bool ret = false;
    spin_lock(&task->unblock_lock);
    if (task->sched_state == WAITING) {
        task->sched_ticks_remaining = 5;
        task->unblock_result = result;
        task->wait_interruptible = false;
        task_set_state_to_running(task->active_processor, task, false);
        ret = false;
    }
    spin_unlock(&task->unblock_lock);
    return ret;
}

void task_set_sig_pending(struct task *task, int signum) {
    task->sig_pending |= (UINT64_C(1) << (signum - UINT64_C(1)));

    if (task->sched_state == WAITING && task->wait_interruptible && !(task_is_sig_blocked(task, signum))) {
        task_unblock(task, -EINTR);
    }
}

void task_unset_sig_pending(struct task *task, int signum) {
    task->sig_pending &= ~(UINT64_C(1) << (signum - UINT64_C(1)));
}

bool task_is_sig_pending(struct task *task, int signum) {
    return task->sig_pending & (UINT64_C(1) << (signum - UINT64_C(1))) ? 1 : 0;
}

void task_do_sig_cont(struct task *task, int signum) {
    if (signum != SIGCONT) {
        return;
    }

    if (task->sched_state == STOPPED) {
        list_for_each_entry_safe(&task->queued_signals, sig, struct queued_signal, queue) {
            enum sig_default_behavior behavior = sig->info.si_signo >= SIGRTMIN ? TERMINATE : sig_defaults[sig->info.si_signo];
            if (behavior == STOP) {
                list_remove(&sig->queue);
                task_free_queued_signal(sig);
            }
        }

        task_unset_sig_pending(task, SIGTSTP);
        task_unset_sig_pending(task, SIGSTOP);
        task_unset_sig_pending(task, SIGTTOU);
        task_unset_sig_pending(task, SIGTTIN);

        task->should_stop = false;
        task_set_state_to_running(task->active_processor, task, !task->in_kernel);
        proc_set_process_state(task->process, PS_CONTINUED, 0, false);
    }
}

static void __task_enqueue_signal_object(struct task *task, struct queued_signal *to_add) {
    int signum = to_add->info.si_signo;
    task_do_sig_cont(task, signum);

    enum sig_default_behavior behavior = signum >= SIGRTMIN ? TERMINATE : sig_defaults[signum];
    if (task->process->sig_state[signum].sa_handler == SIG_IGN ||
        (task->process->sig_state[signum].sa_handler == SIG_DFL && behavior == IGNORE)) {
        task_free_queued_signal(to_add);
        goto finish;
    }

    list_for_each_entry(&task->queued_signals, queued_signal, struct queued_signal, queue) {
        if (queued_signal->info.si_signo > signum) {
            list_prepend(&queued_signal->queue, &to_add->queue);
            goto finish;
        }
    }

    list_append(&task->queued_signals, &to_add->queue);

finish:
    task_set_sig_pending(task, signum);
}

void task_enqueue_signal_object(struct task *task, struct queued_signal *to_add) {
    spin_lock(&task->sig_lock);
    __task_enqueue_signal_object(task, to_add);
    spin_unlock(&task->sig_lock);
}

void task_enqueue_signal(struct task *task, int signum, void *val, bool is_sigqueue) {
    spin_lock(&task->sig_lock);
    task_do_sig_cont(task, signum);

    enum sig_default_behavior behavior = signum >= SIGRTMIN ? TERMINATE : sig_defaults[signum];
    if (task->process->sig_state[signum].sa_handler == SIG_IGN ||
        (task->process->sig_state[signum].sa_handler == SIG_DFL && behavior == IGNORE)) {
        goto task_enqueue_signal_end;
    }

    if (signum < SIGRTMIN && task_is_sig_pending(task, signum)) {
        goto task_enqueue_signal_end;
    }

    if (!(task->process->sig_state[signum].sa_flags & SA_SIGINFO)) {
        task_set_sig_pending(task, signum);
        goto task_enqueue_signal_end;
    }

    struct queued_signal *to_add = malloc(sizeof(struct queued_signal));
    to_add->info.si_signo = signum;
    to_add->info.si_code = is_sigqueue ? SI_QUEUE : SI_USER;
    to_add->info.si_addr = NULL;
    to_add->info.si_errno = 0;
    to_add->info.si_pid = get_current_task()->process->pid;
    to_add->info.si_uid = get_current_task()->process->uid;
    to_add->info.si_status = 0;
    to_add->info.si_band = 0;
    to_add->info.si_value.sival_ptr = val;
    to_add->flags = 0;

    __task_enqueue_signal_object(task, to_add);

task_enqueue_signal_end:
    spin_unlock(&task->sig_lock);
}

void task_free_queued_signal(struct queued_signal *queued_signal) {
    if (queued_signal->flags & QUEUED_SIGNAL_DONT_FREE_FLAG) {
        queued_signal->flags &= ~QUEUED_SIGNAL_DONT_FREE_FLAG;
    } else {
        free(queued_signal);
    }
}

void task_dequeue_signal(struct task *task, struct queued_signal *queued_signal) {
    struct queued_signal *next = list_next_entry(&task->queued_signals, &queued_signal->queue, struct queued_signal, queue);
    if (!next || next->info.si_signo != queued_signal->info.si_signo) {
        task_unset_sig_pending(task, queued_signal->info.si_signo);
    }

    list_remove(&queued_signal->queue);
    task_free_queued_signal(queued_signal);
}

struct queued_signal *task_first_queued_signal(struct task *task) {
    list_for_each_entry(&task->queued_signals, queued_signal, struct queued_signal, queue) {
        if (!task_is_sig_blocked(task, queued_signal->info.si_signo)) {
            return queued_signal;
        }
    }
    return NULL;
}

int task_get_next_sig(struct task *task) {
    if (!task->sig_pending) {
        return -1; // Indicates we did nothing
    }

    for (size_t i = 1; i < _NSIG; i++) {
        if (task_is_sig_pending(task, i) && !task_is_sig_blocked(task, i)) {
            return i;
        }
    }

    return -1;
}

void proc_notify_parent(struct process *parent) {
    proc_set_sig_pending(parent, SIGCHLD);
}

void task_do_sigs_if_needed(struct task *task) {
    // No need to do signals if we should exit anyway.
    if (task->should_exit) {
        return;
    }

    spin_lock(&task->sig_lock);
    int sig = task_get_next_sig(task);
    if (sig != -1) {
        task_do_sig(task, sig);
    }
    spin_unlock(&task->sig_lock);
}

void task_stop(struct task *task) {
    local_sched_remove_task(task->active_processor, task);
    task->sched_state = STOPPED;
}

void task_exit(struct task *task) {
    // It may be wise to send an IPI to the processor instead.
    local_sched_remove_task(task->active_processor, task);
    task->sched_state = EXITING;
    proc_schedule_task_for_destruction(task);
}

void task_set_state_to_waiting(struct task *task) {
    local_sched_remove_task(task->active_processor, task);
    task->sched_state = WAITING;
}

void task_set_state_to_running(struct processor *processor, struct task *task, bool interruptible) {
    task->sched_state = interruptible ? RUNNING_INTERRUPTIBLE : RUNNING_UNINTERRUPTIBLE;
    local_sched_add_task(processor, task);
}

void task_set_state_to_stopped(struct task *task) {
    if (task->sched_state == STOPPED) {
        return;
    }

    if (task->sched_state == WAITING && task->wait_interruptible) {
        // Defer exit state change until after the blocking code has a chance
        // to clean up
        task_unblock(task, -EINTR);
        return;
    }

    if (task->in_kernel) {
        task->should_stop = true;
    } else {
        task_stop(task);
    }
}

void task_set_state_to_exiting(struct task *task) {
    if (task->sched_state == EXITING) {
        return;
    }

    if (task->sched_state == WAITING && task->wait_interruptible) {
        // Defer exit state change until after the blocking code has a chance
        // to clean up
        task_unblock(task, -EINTR);
        return;
    }

    if (task->in_kernel) {
        task->should_exit = true;
    } else {
        task_exit(task);
    }
}

void task_do_sig(struct task *task, int signum) {
    assert(get_current_task() == task);

#ifdef TASK_SIGNAL_DEBUG
    debug_log("Doing signal: [ %d, %s ]\n", task->process->pid, strsignal(signum));
#endif /* TASK_SIGNAL_DEBUG */

    if (task->process->sig_state[signum].sa_handler == SIG_IGN) {
        task_unset_sig_pending(task, signum);
        return;
    }

    if (task->process->sig_state[signum].sa_handler != SIG_DFL) {
        task_do_sig_handler(task, signum);
        return;
    }

    enum sig_default_behavior behavior = signum >= SIGRTMIN ? TERMINATE : sig_defaults[signum];
    task_unset_sig_pending(task, signum);
    switch (behavior) {
        case TERMINATE_AND_DUMP:
            elf64_stack_trace(task, false);
#ifdef __x86_64__
            if (task == get_current_task() && atomic_load(&task->process->should_profile)) {
                spin_lock(&task->process->profile_buffer_lock);
                proc_record_profile_stack(NULL);
                spin_unlock(&task->process->profile_buffer_lock);
            }
#endif
            // Fall through
        case TERMINATE:
            if (task->sched_state == EXITING) {
                break;
            }
            // FIXME: we cant take a mutex if this is called from the scheduler
            mutex_lock(&task->process->lock);
            exit_process(task->process, task);
            proc_set_process_state(task->process, PS_TERMINATED, signum, true);
            mutex_unlock(&task->process->lock);
            task_set_state_to_exiting(task);
            break;
        case STOP:
            if (task->sched_state == STOPPED) {
                break;
            }
            task_set_state_to_stopped(task);
            proc_set_process_state(task->process, PS_STOPPED, signum, false);
            break;
        case IGNORE:
            break;
        default:
            assert(false);
            break;
    }
}

const char *task_state_to_string(enum sched_state state) {
    switch (state) {
        case RUNNING_INTERRUPTIBLE:
            return "R (running)";
        case RUNNING_UNINTERRUPTIBLE:
            return "U (running uninterruptible)";
        case WAITING:
            return "W (waiting)";
        case EXITING:
            return "E (exiting)";
        case STOPPED:
            return "S (stopped)";
        default:
            return "? (Unknown)";
    }
}

bool task_is_sig_blocked(struct task *task, int signum) {
    assert(signum >= 1 && signum < _NSIG);
    return task->sig_mask & (UINT64_C(1) << (signum - UINT64_C(1)));
}
