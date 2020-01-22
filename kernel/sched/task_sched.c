#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>
#include <kernel/util/validators.h>

// #define ROBUST_USER_MUTEX_DEBUG
// #define SCHED_DEBUG

static struct task *list_start = NULL;
static struct task *list_end = NULL;
extern struct task initial_kernel_task;
extern struct task *current_task;
static spinlock_t task_list_lock = SPINLOCK_INITIALIZER;

void init_task_sched() {
    // This only becomes needed after scheduling is enabled
    init_proc_state();
    init_processes();
    init_user_mutexes();

    arch_init_task_sched();
}

void sched_add_task(struct task *task) {
    spin_lock(&task_list_lock);

    if (list_start == NULL) {
        list_start = list_end = task;
        task->prev = task->next = task;

        spin_unlock(&task_list_lock);
        return;
    }

    task->prev = list_end;
    task->next = list_start;

    list_end->next = task;
    list_start->prev = task;
    list_end = task;

    spin_unlock(&task_list_lock);
}

void sched_remove_task(struct task *task) {
    spin_lock(&task_list_lock);

    if (list_start == NULL) {
        spin_unlock(&task_list_lock);
        return;
    }

    struct task *current = list_start;

    while (current->next != task) {
        current = current->next;
    }

    if (task == list_end) {
        list_end = task->prev;
    }

    if (task == list_start) {
        list_start = task->next;
    }

    current->next = current->next->next;
    current->next->prev = current;

    spin_unlock(&task_list_lock);
}

/* Must be called from unpremptable context */
void sched_run_next() {
    struct task *current = get_current_task();
    if (current == &initial_kernel_task) {
        current = list_start;
    }

    // Task signals
    struct task *task = list_start;
    do {
        // Don't send the signals if the task is running UNINTERRUPTABLY, or if it is just going to exit anyway
        if (task->sched_state == RUNNING_UNINTERRUPTIBLE || task->sched_state == EXITING) {
            continue;
        }

        int sig;
        while ((sig = task_get_next_sig(task)) != -1) {
            task_do_sig(task, sig);
        }
    } while ((task = task->next) != list_start);

    struct task *to_run = current->next;
    struct task *start = to_run;
    while (to_run->sched_state != RUNNING_INTERRUPTIBLE && to_run->sched_state != RUNNING_UNINTERRUPTIBLE) {
        struct task *next = to_run->next;
        if (to_run->sched_state == EXITING) {
            struct task *to_remove = to_run;

            if (to_remove == list_end) {
                list_end = to_remove->prev;
            }

            if (to_remove == list_start) {
                list_start = to_remove->next;
            }

            if (to_remove == start) {
                start = to_remove->next;
            }

            struct task *prev_save = to_remove->prev;
            prev_save->next = to_remove->next;
            prev_save->next->prev = prev_save;

            free_task(to_remove, true);
        } else if (to_run->blocking && to_run->sched_state == WAITING) {
            struct task *current_save = current_task;
            current_task = to_run;
            if (to_run->block_info.should_unblock(&to_run->block_info)) {
                to_run->blocking = false;
                current_task = current_save;
                to_run->sched_state = RUNNING_UNINTERRUPTIBLE;
                break;
            }
            current_task = current_save;
        }

        // Skip taskes that are sleeping
        to_run = next;

        // There were no other tasks to run
        if (to_run == start) {
            // This means we need to run the idle task
            if (to_run->sched_state != RUNNING_UNINTERRUPTIBLE && to_run->sched_state != RUNNING_INTERRUPTIBLE) {
                to_run = &initial_kernel_task;
            }

            break;
        }
    }

#ifdef SCHED_DEBUG
    if (to_run != &initial_kernel_task) {
        debug_log("Running task: [ %d:%d ]\n", to_run->tid, to_run->process->pid);
    }
#endif /* SCHED_DEBUG */

    assert(to_run->sched_state != WAITING);
    run_task(to_run);
}

struct task *find_task_for_process(pid_t pid) {
    spin_lock(&task_list_lock);
    struct task *found = NULL;
    struct task *task = list_start;

    do {
        if (task->process->pid == pid) {
            found = task;
            break;
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);
    return found;
}

int signal_process_group(pid_t pgid, int signum) {
    spin_lock(&task_list_lock);

    bool signalled_self = false;
    bool signalled_anything = false;
    struct task *task = list_start;
    do {
        // FIXME: only signal 1 task per process
        if (task->process->pgid == pgid) {
            if (signum != 0) {
                task_set_sig_pending(task, signum);

                if (task == get_current_task()) {
                    signalled_self = true;
                }
            }
            signalled_anything = true;
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);

    if (signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == WAITING) {
            yield_signal();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

int signal_task(int tgid, int tid, int signum) {
    spin_lock(&task_list_lock);

    bool signalled_self = false;
    bool signalled_anything = false;
    struct task *task = list_start;
    do {
        if (task->process->pid == tgid && task->tid == tid) {
            if (signum != 0) {
                debug_log("Signaling: [ %d, %d ]\n", task->process->pid, signum);
                task_set_sig_pending(task, signum);

                if (task == get_current_task()) {
                    signalled_self = true;
                }
            }
            signalled_anything = true;

            break;
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);

    if (signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == WAITING) {
            yield_signal();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

int signal_process(pid_t pid, int signum) {
    spin_lock(&task_list_lock);

    bool signalled_self = false;
    bool signalled_anything = false;
    struct task *task = list_start;
    do {
        // Maybe should only do it once instead of in a loop
        if (task->process->pid == pid) {
            if (signum != 0) {
                debug_log("Signaling: [ %d, %d ]\n", task->process->pid, signum);
                task_set_sig_pending(task, signum);

                if (task == get_current_task()) {
                    signalled_self = true;
                }
            }
            signalled_anything = true;
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);

    if (signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == WAITING) {
            yield_signal();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

void exit_process(struct process *process) {
    struct task *task = list_start;
    do {
        if (task->process == process) {
            task->sched_state = EXITING;

#ifdef ROBUST_USER_MUTEX_DEBUG
            debug_log("Locked robust mutex list head pointer: [ %p ]\n", task->locked_robust_mutex_list_head);
#endif /* ROBUST_USER_MUTEX_DEBUG */
            if (!validate_read_or_null(task->locked_robust_mutex_list_head, sizeof(struct __locked_robust_mutex_node *))) {
                struct __locked_robust_mutex_node *node = task->locked_robust_mutex_list_head ? *task->locked_robust_mutex_list_head : NULL;
#ifdef ROBUST_USER_MUTEX_DEBUG
                debug_log("Locked robust mutex list head: [ %p ]\n", node);
#endif /* ROBUST_USER_MUTEX_DEBUG */

                while (!validate_read(node, sizeof(struct __locked_robust_mutex_node))) {
#ifdef ROBUST_USER_MUTEX_DEBUG
                    debug_log("Checking mutex: [ %p, %p, %d, %p, %p ]\n", node, node->__protected, node->__in_progress_flags, node->__prev,
                              node->__next);
#endif /* ROBUST_USER_MUTEX_DEBUG */
                    if ((node->__in_progress_flags == 0) || (node->__in_progress_flags == ROBUST_MUTEX_IS_VALID_IF_VALUE &&
                                                             *node->__protected == (unsigned int) node->__in_progress_value)) {
                        struct user_mutex *um = get_user_mutex_locked_with_waiters_or_else_write_value(node->__protected, MUTEX_OWNER_DIED);
                        if (um != NULL) {
                            *node->__protected = MUTEX_OWNER_DIED;
                            wake_user_mutex(um, 1);
                            unlock_user_mutex(um);
                        }
                    }

                    node = node->__next;
                }
            }
        }
    } while ((task = task->next) != list_start);
}