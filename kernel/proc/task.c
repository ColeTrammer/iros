#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/vfs.h>
#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/pid.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

// #define TASK_SIGNAL_DEBUG

struct task *current_task;
struct task initial_kernel_task;
struct process initial_kernel_process;

/* Copying args and envp is necessary because they could be saved on the program stack we are about to overwrite */
uintptr_t map_program_args(uintptr_t start, char **argv, char **envp) {
    size_t argc = 0;
    size_t args_str_length = 0;
    while (argv[argc++] != NULL) {
        args_str_length += strlen(argv[argc - 1]) + 1;
    }

    size_t envc = 0;
    size_t env_str_length = 0;
    while (envp[envc++] != NULL) {
        env_str_length += strlen(envp[envc - 1]) + 1;
    }

    char **args_copy = calloc(argc, sizeof(char **));
    char **envp_copy = calloc(envc, sizeof(char **));

    char *args_buffer = malloc(args_str_length);
    char *env_buffer = malloc(env_str_length);

    ssize_t j = 0;
    ssize_t i = 0;
    while (argv[i] != NULL) {
        ssize_t last = j;
        while (argv[i][j - last] != '\0') {
            args_buffer[j] = argv[i][j - last];
            j++;
        }
        args_buffer[j++] = '\0';
        args_copy[i++] = args_buffer + last;
    }
    args_copy[i] = NULL;

    j = 0;
    i = 0;
    while (envp[i] != NULL) {
        ssize_t last = j;
        while (envp[i][j - last] != '\0') {
            env_buffer[j] = envp[i][j - last];
            j++;
        }
        env_buffer[j++] = '\0';
        envp_copy[i++] = env_buffer + last;
    }
    envp_copy[i] = NULL;

    char **argv_start = (char **) (start - sizeof(char **));

    size_t count = argc + envc;
    char *args_start = (char *) (argv_start - count);

    for (i = 0; args_copy[i] != NULL; i++) {
        args_start -= strlen(args_copy[i]) + 1;
        strcpy(args_start, args_copy[i]);
        argv_start[i - argc] = args_start;
    }

    argv_start[0] = NULL;

    for (i = 0; envp_copy[i] != NULL; i++) {
        args_start -= strlen(envp_copy[i]) + 1;
        strcpy(args_start, envp_copy[i]);
        argv_start[i - count] = args_start;
    }

    argv_start[-(argc + 1)] = NULL;

    args_start = (char *) ((((uintptr_t) args_start) & ~0x7) - 0x08);

    args_start -= sizeof(size_t);
    *((size_t *) args_start) = argc - 1;
    args_start -= sizeof(char **);
    *((char ***) args_start) = argv_start - argc;
    args_start -= sizeof(char **);
    *((char ***) args_start) = argv_start - count;

    free(args_copy);
    free(envp_copy);
    free(args_buffer);
    free(env_buffer);

    return (uintptr_t) args_start;
}

static spinlock_t tid_lock;
static int tid_counter = 1;

int get_next_tid() {
    spin_lock(&tid_lock);

    int tid = tid_counter++;

    spin_unlock(&tid_lock);
    return tid;
}

void init_kernel_task() {
    current_task = &initial_kernel_task;

    current_task->process = &initial_kernel_process;
    arch_init_kernel_task(current_task);

    initial_kernel_task.tid = get_next_tid();
    initial_kernel_task.kernel_task = true;
    initial_kernel_task.process->pid = 1;
    init_spinlock(&initial_kernel_process.lock);
    initial_kernel_task.sched_state = RUNNING_UNINTERRUPTIBLE;
    initial_kernel_task.next = NULL;
    initial_kernel_task.process->pgid = 1;
    initial_kernel_task.process->ppid = 1;
    initial_kernel_task.process->tty = -1;
}

struct task *load_kernel_task(uintptr_t entry) {
    struct task *task = calloc(1, sizeof(struct task));
    struct process *process = calloc(1, sizeof(struct process));
    task->process = process;

    task->process->pid = get_next_pid();
    init_spinlock(&process->lock);
    proc_add_process(process);
    task->process->pgid = task->process->pid;
    task->process->ppid = initial_kernel_task.process->pid;
    task->process->process_memory = NULL;
    task->kernel_task = true;
    task->sched_state = RUNNING_UNINTERRUPTIBLE;
    task->process->cwd = malloc(2);
    task->process->tty = -1;
    task->tid = get_next_tid();
    strcpy(task->process->cwd, "/");
    task->next = NULL;

    arch_load_kernel_task(task, entry);

    debug_log("Loaded Kernel Task: [ %d ]\n", task->process->pid);
    return task;
}

struct task *load_task(const char *file_name) {
    int error = 0;
    struct file *program = fs_open(file_name, O_RDONLY, &error);
    assert(program != NULL && error == 0);

    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    struct task *task = calloc(1, sizeof(struct task));
    struct process *process = calloc(1, sizeof(struct process));
    task->process = process;

    task->process->inode_dev = program->device;
    task->process->inode_id = program->inode_idenifier;

    fs_close(program);

    assert(elf64_is_valid(buffer));

    task->tid = get_next_tid();
    task->process->pid = get_next_pid();
    init_spinlock(&process->lock);
    proc_add_process(process);
    task->process->pgid = task->process->pid;
    task->process->ppid = initial_kernel_task.process->pid;
    task->process->process_memory = NULL;
    task->kernel_task = false;
    task->sched_state = RUNNING_INTERRUPTIBLE;
    task->process->cwd = malloc(2);
    task->process->tty = -1;
    strcpy(task->process->cwd, "/");
    task->next = NULL;

    uintptr_t old_paging_structure = get_current_paging_structure();
    uintptr_t structure = create_paging_structure(task->process->process_memory, false);
    load_paging_structure(structure);

    elf64_load_program(buffer, length, task);
    elf64_map_heap(buffer, task);

    struct vm_region *task_stack = calloc(1, sizeof(struct vm_region));
    task_stack->flags = VM_USER | VM_WRITE | VM_NO_EXEC | VM_STACK;
    task_stack->type = VM_TASK_STACK;
    task_stack->start = find_first_kernel_vm_region()->start - PAGE_SIZE - 2 * 1024 * 1024;
    task_stack->end = task_stack->start + 2 * 1024 * 1024;
    task->process->process_memory = add_vm_region(task->process->process_memory, task_stack);
    map_page(task_stack->end - PAGE_SIZE, task_stack->flags);

    struct vm_region *guard_page = calloc(1, sizeof(struct vm_region));
    guard_page->flags = VM_PROT_NONE;
    guard_page->type = VM_TASK_STACK_GUARD;
    guard_page->start = task_stack->end;
    guard_page->end = guard_page->start + PAGE_SIZE;
    task->process->process_memory = add_vm_region(task->process->process_memory, guard_page);

    arch_load_task(task, elf64_get_entry(buffer));
    free(buffer);

    load_paging_structure(old_paging_structure);

    task->process->files[0] = fs_open("/dev/serial", O_RDWR, NULL);
    task->process->files[1] = fs_open("/dev/serial", O_RDWR, NULL);
    task->process->files[2] = fs_open("/dev/serial", O_RDWR, NULL);

    debug_log("Loaded Task: [ %d, %s ]\n", task->process->pid, file_name);
    return task;
}

/* Must be called from unpremptable context */
void run_task(struct task *task) {
    arch_run_task(task);
}

struct task *get_current_task() {
    return current_task;
}

/* Must be called from unpremptable context */
void free_task(struct task *task, bool free_paging_structure) {
    struct task *current_save = current_task;
    current_task = task;

    struct __locked_robust_mutex_node *node = task->locked_robust_mutex_stack_top ? *task->locked_robust_mutex_stack_top : NULL;
    while (node) {
        if ((node->__in_progress_flags == 0) ||
            (node->__in_progress_flags == ROBUST_MUTEX_IS_VALID_IF_VALUE &&
             *node->__protected == (unsigned int) node->__in_progress_value) ||
            (node->__in_progress_flags == ROBUST_MUTEX_IS_VALID_IF_NOT_VALUE &&
             *node->__protected != (unsigned int) node->__in_progress_value)) {
            *node->__protected = MUTEX_OWNER_DIED;
        }

        node = node->__prev;
    }

    arch_free_task(task, free_paging_structure);

    proc_drop_process(task->process, free_paging_structure);
    free(task);
    current_task = current_save;
}

void task_set_sig_pending(struct task *task, int signum) {
    task->sig_pending |= (1U << signum);
}

void task_unset_sig_pending(struct task *task, int signum) {
    task->sig_pending &= ~(1U << signum);
}

int task_get_next_sig(struct task *task) {
    if (!task->sig_pending) {
        return -1; // Indicates we did nothing
    }

    for (size_t i = 1; i < _NSIG; i++) {
        if ((task->sig_pending & (1U << i)) && !task_is_sig_blocked(task, i)) {
            return i;
        }
    }

    return -1;
}

void proc_notify_parent(pid_t child_pid) {
    struct process *child = find_by_pid(child_pid);
    struct process *parent = find_by_pid(child->ppid);

    if (parent == NULL) {
        parent = &initial_kernel_process;
    }

    proc_set_sig_pending(parent, SIGCHLD);
}

void task_do_sigs_if_needed(struct task *task) {
    int sig = task_get_next_sig(task);
    if (sig != -1) {
        task_do_sig(task, sig);
    }
}

enum sig_default_behavior { TERMINATE, TERMINATE_AND_DUMP, IGNORE, STOP, CONTINUE, INVAL };

static enum sig_default_behavior sig_defaults[_NSIG] = {
    INVAL,              // INVAL
    TERMINATE,          // SIGHUP
    TERMINATE,          // SIGINT
    TERMINATE_AND_DUMP, // SIGQUIT
    TERMINATE_AND_DUMP, // SIGBUS
    TERMINATE_AND_DUMP, // SIGTRAP
    TERMINATE_AND_DUMP, // SIGABRT
    CONTINUE,           // SIGCONT
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
    TERMINATE,          // INVAL
    TERMINATE,          // INVAL
    TERMINATE           // __PTHREAD_CANCEL_SIGNAL
};

void task_do_sig(struct task *task, int signum) {
#ifdef TASK_SIGNAL_DEBUG
    debug_log("Doing signal: [ %d, %d ]\n", task->pid, signum);
#endif /* TASK_SIGNAL_DEBUG */

    task_unset_sig_pending(task, signum);

    if (task->process->sig_state[signum].sa_handler == SIG_IGN) {
        return;
    }

    if (task->process->sig_state[signum].sa_handler != SIG_DFL) {
        task_do_sig_handler(task, signum);
    }

    assert(sig_defaults[signum] != INVAL);
    switch (sig_defaults[signum]) {
        case TERMINATE_AND_DUMP:
            elf64_stack_trace(task);
            // Fall through
        case TERMINATE:
            if (task->sched_state == EXITING) {
                break;
            }
            exit_process(task->process);
            invalidate_last_saved(task);
            proc_add_message(task->process->pid, proc_create_message(STATE_INTERRUPTED, signum));
            break;
        case STOP:
            if (task->sched_state == WAITING) {
                break;
            }
            task->sched_state = WAITING;
            proc_add_message(task->process->pid, proc_create_message(STATE_STOPPED, signum));
            break;
        case CONTINUE:
            if (task->sched_state == RUNNING_INTERRUPTIBLE || task->sched_state == RUNNING_UNINTERRUPTIBLE) {
                break;
            }
            task->sched_state = RUNNING_INTERRUPTIBLE;
            proc_add_message(task->process->pid, proc_create_message(STATE_CONTINUED, signum));
            break;
        case IGNORE:
            break;
        default:
            assert(false);
            break;
    }
}

bool task_is_sig_blocked(struct task *task, int signum) {
    assert(signum >= 1 && signum < _NSIG);
    return task->sig_mask & (1U << (signum - 1));
}