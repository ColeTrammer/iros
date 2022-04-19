#ifndef _KERNEL_PROC_TASK_H
#define _KERNEL_PROC_TASK_H 1

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/iros.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/vm_region.h>
#include <kernel/proc/process.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/list.h>
#include <kernel/util/mutex.h>
#include <kernel/util/spinlock.h>

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
    struct list_node sched_list;

    // Inline doublely-linked list connecting all tasks in a process
    struct list_node process_list;

    // Inline list pointer used by process finializer
    struct task *finialize_queue_next;

    // Pointer to the processor this task will run on. Currently, this will stay the same
    // throughout the task's lifetime.
    struct processor *active_processor;

    // Pointer to the saved user task state if this task task is a user task and is
    // currently in the kernel (syscall or page fault handler).
    struct task_state *user_task_state;

    struct vm_region *kernel_stack;

    enum sched_state sched_state;

    sigset_t saved_sig_mask;

    sigset_t sig_mask;
    sigset_t sig_pending;
    struct list_node queued_signals;
    spinlock_t sig_lock;

    int tid;
    int sched_ticks_remaining;

    // Used to restart interrupted signals.
    int last_system_call;

    bool kernel_task : 1;
    bool in_kernel : 1;
    bool in_sigsuspend : 1;

    int unblock_result;
    spinlock_t unblock_lock;
    bool should_exit : 1;
    bool should_stop : 1;
    bool wait_interruptible : 1;

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

void task_exit(struct task *task);
void task_stop(struct task *task);
void free_task(struct task *task, bool free_paging_structure);
void arch_free_task(struct task *task, bool free_paging_structure);

pid_t proc_fork(void);
int proc_execve(char *path, char **argv, char **envp);
int proc_waitpid(pid_t pid, int *status, int flags);
pid_t proc_getppid(struct process *process);
void start_userland(void);

void proc_clone_program_args(struct process *process, char **prepend_argv, char **argv, char **envp);
uintptr_t map_program_args(uintptr_t start, struct args_context *context, struct initial_process_info *info, struct task *task);
char *arch_setup_program_args(struct task *task, char *args_start, struct initial_process_info *info, size_t argc, char **argv,
                              char **envp);
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

void arch_task_switch_from_kernel_to_user_mode(struct task *task);
void arch_task_set_thread_self_pointer(struct task *task, void *thread_self_pointer);
void arch_sys_create_task(struct task *task, uintptr_t entry, uintptr_t new_sp, uintptr_t return_address, void *arg);
void arch_task_prepare_to_restart_sys_call(struct task *task);

void task_yield_if_state_changed(struct task *task);
void task_do_sigs_if_needed(struct task *task);

bool task_unblock(struct task *task, int ret);
void task_set_state_to_exiting(struct task *task);
void task_set_state_to_stopped(struct task *task);
void task_set_state_to_waiting(struct task *task);
void task_set_state_to_running(struct processor *processor, struct task *task, bool interruptible);

const char *task_state_to_string(enum sched_state state);
bool task_in_kernel(struct task *task);

extern struct task initial_kernel_task;

static inline void __wait_cancel(struct task *task) {
    task->wait_interruptible = false;
    task_set_state_to_running(task->active_processor, task, false);
    enable_preemption();
}

static inline int __wait_prepare(struct task *task, bool interruptible) {
    disable_preemption();

    task->wait_interruptible = interruptible;
    task_set_state_to_waiting(task);

    if (interruptible) {
        spin_lock(&task->sig_lock);
        int sig = task_get_next_sig(task);
        spin_unlock(&task->sig_lock);
        if (sig != -1) {
            __wait_cancel(task);
            return -EINTR;
        }
    }
    return 0;
}

static inline int __wait_do() {
    __enable_preemption();
    return kernel_yield();
}

#define wait_prepare(task)               __wait_prepare(task, false)
#define wait_prepare_interruptible(task) __wait_prepare(task, true)
#define wait_do(task)                    __wait_do()

#define __wait_for(_task, cond, wq, begin_wait, end_wait, interruptible, lock_wq)                                   \
    ({                                                                                                              \
        struct wait_queue_entry __entry = { .task = (_task) };                                                      \
        ___wait_for(_task, &__entry, cond, wq, begin_wait, end_wait, wait_do(_task), interruptible, lock_wq, true); \
    })

#define ___wait_for(_task, __entry, cond, wq, begin_wait, end_wait, do_block, interruptible, lock_wq, queue_task) \
    ({                                                                                                            \
        int __ret = 0;                                                                                            \
        bool __queue_task = queue_task;                                                                           \
        for (;;) {                                                                                                \
            __ret = __wait_prepare(_task, interruptible);                                                         \
            if (__ret) {                                                                                          \
                begin_wait;                                                                                       \
                break;                                                                                            \
            }                                                                                                     \
            if (cond) {                                                                                           \
                __wait_cancel(_task);                                                                             \
                break;                                                                                            \
            }                                                                                                     \
            if (__queue_task && lock_wq) {                                                                        \
                spin_lock(&(wq)->lock);                                                                           \
            }                                                                                                     \
            if (__queue_task) {                                                                                   \
                __wait_queue_enqueue_entry(wq, __entry, __func__);                                                \
            }                                                                                                     \
            begin_wait;                                                                                           \
            if (__queue_task && lock_wq) {                                                                        \
                spin_unlock_no_irq_restore(&(wq)->lock);                                                          \
            }                                                                                                     \
            __queue_task = false;                                                                                 \
            __ret = do_block;                                                                                     \
            if (__ret) {                                                                                          \
                break;                                                                                            \
            }                                                                                                     \
            end_wait;                                                                                             \
        }                                                                                                         \
        if (!__queue_task) {                                                                                      \
            if (lock_wq) {                                                                                        \
                wait_queue_dequeue_entry(wq, __entry, __func__);                                                  \
            } else {                                                                                              \
                __wait_queue_dequeue_entry(wq, __entry, __func__);                                                \
            }                                                                                                     \
        }                                                                                                         \
        __ret;                                                                                                    \
    })

#define wait_for(task, cond, wq, begin_wait, end_wait)               __wait_for(task, cond, wq, begin_wait, end_wait, false, true)
#define wait_for_interruptible(task, cond, wq, begin_wait, end_wait) __wait_for(task, cond, wq, begin_wait, end_wait, true, true)

#define wait_for_with_spinlock(task, cond, wq, lock) \
    __wait_for(task, cond, wq, spin_unlock_no_irq_restore(lock), spin_lock(lock), false, true)
#define wait_for_with_spinlock_interruptible(task, cond, wq, lock) \
    __wait_for(task, cond, wq, spin_unlock_no_irq_restore(lock), spin_lock(lock), true, true)

#define wait_for_with_mutex(task, cond, wq, lock)               __wait_for(task, cond, wq, mutex_unlock(lock), mutex_lock(lock), false, true)
#define wait_for_with_mutex_interruptible(task, cond, wq, lock) __wait_for(task, cond, wq, mutex_unlock(lock), mutex_lock(lock), true, true)

#define wait_simple(_task, wq)                               \
    ({                                                       \
        wait_prepare(_task);                                 \
        spin_lock(&(wq)->lock);                              \
        struct wait_queue_entry __entry = { .task = _task }; \
        __wait_queue_enqueue_entry(wq, &__entry, __func__);  \
        spin_unlock_no_irq_restore(&(wq)->lock);             \
        int __ret = wait_do(_task);                          \
        wait_queue_dequeue_entry(wq, &__entry, __func__);    \
        __ret;                                               \
    })

#endif /* _KERNEL_PROC_TASK_H */
