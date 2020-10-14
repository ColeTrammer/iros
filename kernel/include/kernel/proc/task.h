#ifndef _KERNEL_PROC_TASK_H
#define _KERNEL_PROC_TASK_H 1

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/os_2.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/blockers.h>
#include <kernel/proc/process.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/list.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(proc/arch_task.h)
// clang-format on

enum sched_state {
    RUNNING_INTERRUPTIBLE,
    RUNNING_UNINTERRUPTIBLE,
    WAITING,
    STOPPED,
    EXITING,
};

struct clock;
struct process;
struct processor;

struct args_context {
    size_t prepend_argc;
    size_t argc;
    size_t envc;
    size_t args_bytes;
    size_t env_bytes;
    char **prepend_args_copy;
    char **args_copy;
    char **envp_copy;
    char *prepend_args_buffer;
    char *args_buffer;
    char *env_buffer;
};

#define QUEUED_SIGNAL_DONT_FREE_FLAG 1

struct queued_signal {
    struct list_node queue;
    int flags;
    siginfo_t info;
};

struct task {
    struct arch_task arch_task;

    // Inline circular doubly-linked list used by the scheduler
    struct task *sched_next;
    struct task *sched_prev;

    // Inline doublely-linked list connecting all tasks in a process
    struct list_node process_list;

    // Inline list pointer used by process finializer
    struct task *finialize_queue_next;

    // Inline list pointer used by wait_queue functions
    struct task *wait_queue_next;

    struct vm_region *kernel_stack;

    enum sched_state sched_state;

    sigset_t sig_mask;
    sigset_t saved_sig_mask;
    sigset_t sig_pending;

    struct list_node queued_signals;

    int tid;

    // Used to restart interrupted signals.
    int last_system_call;

    bool kernel_task : 1;
    bool in_kernel : 1;
    bool in_sigsuspend : 1;
    bool blocking : 1;
    bool should_exit : 1;
    bool wait_interruptible : 1;

    struct block_info block_info;

    struct clock *task_clock;

    struct process *process;
    struct __locked_robust_mutex_node **locked_robust_mutex_list_head;

    struct arch_fpu_state fpu;
};

void init_kernel_process(void);

void init_idle_task(struct processor *processor);
void arch_init_idle_task(struct task *kernel_task, struct processor *processor);

struct task *load_kernel_task(uintptr_t entry, const char *name);
void arch_load_kernel_task(struct task *task, uintptr_t entry);

void run_task(struct task *task);
void arch_run_task(struct task *task);

void free_task(struct task *task, bool free_paging_structure);
void arch_free_task(struct task *task, bool free_paging_structure);

pid_t proc_fork(void);
int proc_execve(char *path, char **argv, char **envp);
int proc_waitpid(pid_t pid, int *status, int flags);
pid_t proc_getppid(struct process *process);
void start_userland(void);

void proc_clone_program_args(struct process *process, char **prepend_argv, char **argv, char **envp);
uintptr_t map_program_args(uintptr_t start, struct args_context *context, struct initial_process_info *info, struct task *task);
void arch_setup_program_args(struct task *task, struct initial_process_info *info, size_t argc, char **argv, char **envp);
void free_program_args(struct args_context *context);

int get_next_tid();

void task_set_sig_pending(struct task *task, int signum);
void task_unset_sig_pending(struct task *task, int signum);
bool task_is_sig_pending(struct task *task, int signum);
int task_get_next_sig(struct task *task);
void task_do_sig(struct task *task, int signum);
void task_do_sig_handler(struct task *task, int signum);
bool task_is_sig_blocked(struct task *task, int signum);
void proc_notify_parent(struct process *child);
void task_enqueue_signal(struct task *task, int signum, void *val, bool was_sigqueue);
void task_enqueue_signal_object(struct task *task, struct queued_signal *sig);
void task_dequeue_signal(struct task *task, struct queued_signal *signal);
void task_free_queued_signal(struct queued_signal *queued_signal);
struct queued_signal *task_first_queued_signal(struct task *task);

void task_yield_if_state_changed(struct task *task);
void task_do_sigs_if_needed(struct task *task);

void task_unblock(struct task *task, int ret);
void task_set_state_to_exiting(struct task *task);

const char *task_state_to_string(enum sched_state state);
bool task_in_kernel(struct task *task);

extern struct task initial_kernel_task;

static inline void __wait_prepare(struct task *task, uint64_t *interrupts_save, bool interruptible) {
    *interrupts_save = disable_interrupts_save();
    task->wait_interruptible = interruptible;
    task->sched_state = WAITING;
}

static inline int __wait_do(uint64_t *interrupts_save) {
    int ret = __kernel_yield();
    interrupts_restore(*interrupts_save);
    return ret;
}

#define wait_prepare(task, save)               __wait_prepare(task, save, false)
#define wait_prepare_interruptible(task, save) __wait_prepare(task, save, true)
#define wait_do(task, save)                    __wait_do(save)

#define __wait_for(task, cond, begin_wait, end_wait, interruptible) \
    ({                                                              \
        int __ret = 0;                                              \
        uint64_t __save;                                            \
        for (;;) {                                                  \
            if (cond) {                                             \
                break;                                              \
            }                                                       \
            __wait_prepare(task, &save, interruptible);             \
            begin_wait;                                             \
            __ret = wait_do(task, &save);                           \
            if (__ret) {                                            \
                break;                                              \
            }                                                       \
            end_wait;                                               \
        }                                                           \
        __ret;                                                      \
    })

#define wait_with_blocker(task)                    \
    ({                                             \
        uint64_t __save;                           \
        task->blocking = true;                     \
        wait_prepare_interruptible(task, &__save); \
        wait_do(task, &__save);                    \
    })

#endif /* _KERNEL_PROC_TASK_H */
