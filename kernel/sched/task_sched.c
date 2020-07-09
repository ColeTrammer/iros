#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/proc/task_finalizer.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>
#include <kernel/util/validators.h>

// #define ROBUST_USER_MUTEX_DEBUG
// #define SCHED_DEBUG

extern struct process initial_kernel_process;

void init_task_sched() {
    // This only becomes needed after scheduling is enabled
    init_proc_state();
    init_processes();
    init_user_mutexes();

    arch_init_task_sched();
}

void sched_add_task(struct task *task) {
    uint64_t save = disable_interrupts_save();

    struct processor *processor = get_current_processor();

    if (processor->sched_list_start == NULL) {
        processor->sched_list_start = processor->sched_list_end = task;
        task->sched_prev = task->sched_next = task;

        interrupts_restore(save);
        return;
    }

    task->sched_prev = processor->sched_list_end;
    task->sched_next = processor->sched_list_start;

    processor->sched_list_end->sched_next = task;
    processor->sched_list_start->sched_prev = task;
    processor->sched_list_end = task;

    interrupts_restore(save);
}

void sched_remove_task(struct task *task) {
    uint64_t save = disable_interrupts_save();

    struct processor *processor = get_current_processor();

    if (processor->sched_list_start == NULL) {
        interrupts_restore(save);
        return;
    }

    struct task *current = processor->sched_list_start;

    while (current->sched_next != task) {
        current = current->sched_next;
    }

    if (task == processor->sched_list_end) {
        processor->sched_list_end = task->sched_prev;
    }

    if (task == processor->sched_list_start) {
        processor->sched_list_start = task->sched_next;
    }

    current->sched_next = current->sched_next->sched_next;
    current->sched_next->sched_prev = current;

    interrupts_restore(save);
}

/* Must be called from unpremptable context */
void sched_run_next() {
    struct processor *processor = get_current_processor();
    struct task *current = get_current_task();

    if (current == get_idle_task()) {
        current = processor->sched_list_start;
    }

    // No tasks in queue
    if (current == NULL) {
        run_task(get_idle_task());
    }

    // Task signals
    struct task *task = processor->sched_list_start;
    do {
        // Don't send the signals if the task is running UNINTERRUPTABLY, or if it is just going to exit anyway
        // FIXME: what to do if the task is stopped? It might be unsafe to send a terminating signal, since it
        //        might leak resources.
        if (task->sched_state == RUNNING_UNINTERRUPTIBLE || task->sched_state == EXITING ||
            (task->sched_state == WAITING && !task->blocking && !task->in_sigsuspend)) {
            continue;
        }

        int sig;
        while ((sig = task_get_next_sig(task)) != -1) {
            struct task *current_save = current;
            processor->current_task = task;
            task_do_sig(task, sig);
            processor->current_task = current_save;
        }
    } while ((task = task->sched_next) != processor->sched_list_start);

    struct task *to_run = current->sched_next;
    struct task *start = to_run;
    while (to_run->sched_state != RUNNING_INTERRUPTIBLE && to_run->sched_state != RUNNING_UNINTERRUPTIBLE) {
        struct task *next = to_run->sched_next;
        if (to_run->sched_state == EXITING) {
            struct task *to_remove = to_run;

            if (to_remove == processor->sched_list_end) {
                processor->sched_list_end = to_remove->sched_prev;
            }

            if (to_remove == processor->sched_list_start) {
                processor->sched_list_start = to_remove->sched_next;
            }

            if (to_remove == start) {
                start = to_remove->sched_next;
            }

            struct task *prev_save = to_remove->sched_prev;
            prev_save->sched_next = to_remove->sched_next;
            prev_save->sched_next->sched_prev = prev_save;

            proc_schedule_task_for_destruction(to_remove);
        } else if (to_run->blocking && to_run->sched_state == WAITING) {
            struct task *current_save = processor->current_task;
            processor->current_task = to_run;
            if (to_run->block_info.should_unblock(&to_run->block_info)) {
                task_interrupt_blocking(to_run, 0);
                processor->current_task = current_save;
                break;
            }
            processor->current_task = current_save;
        }

        // Skip taskes that are sleeping
        to_run = next;

        // There were no other tasks to run
        if (to_run == start) {
            // This means we need to run the idle task
            if (to_run->sched_state != RUNNING_UNINTERRUPTIBLE && to_run->sched_state != RUNNING_INTERRUPTIBLE) {
                to_run = get_idle_task();
            }

            break;
        }
    }

#ifdef SCHED_DEBUG
    debug_log("Running task: [ %d, %d:%d ]\n", processor->id, to_run->tid, to_run->process->pid);
#endif /* SCHED_DEBUG */

    assert(to_run->sched_state != WAITING && to_run->sched_state != STOPPED && to_run->sched_state != EXITING);
    run_task(to_run);
}

struct signal_process_group_closure {
    bool signalled_self;
    bool signalled_anything;
    int signum;
};

static void signal_process_group_iter(struct process *process, void *_cls) {
    struct signal_process_group_closure *cls = _cls;

    mutex_lock(&process->lock);

    // FIXME: dispatch signals to a different task than the first if it makes sense.
    struct task *task = process->task_list;
    if (cls->signum != 0) {
        debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(cls->signum));
        task_enqueue_signal(task, cls->signum, NULL, false);

        if (task == get_current_task() && !task_is_sig_blocked(task, cls->signum)) {
            cls->signalled_self = true;
        }
    }
    cls->signalled_anything = true;

    mutex_unlock(&process->lock);
}

int signal_process_group(pid_t pgid, int signum) {
    struct signal_process_group_closure cls = { .signalled_anything = false, .signalled_self = false, .signum = signum };

    proc_for_each_with_pgid(pgid, signal_process_group_iter, &cls);

    if (cls.signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            sched_run_next();
        }
        interrupts_restore(save);
    }

    return cls.signalled_anything ? 0 : -ESRCH;
}

struct task *find_by_tid(int tgid, int tid) {
    struct process *process;
    if (tgid == 1) {
        process = &initial_kernel_process;
    } else {
        process = find_by_pid(tgid);
    }

    if (!process) {
        return NULL;
    }

    mutex_lock(&process->lock);
    for (struct task *task = process->task_list; task; task = task->process_next) {
        if (task->tid == tid) {
            mutex_unlock(&process->lock);
            return task;
        }
    }
    mutex_unlock(&process->lock);

    return NULL;
}

int signal_task(int tgid, int tid, int signum) {
    bool signalled_self = false;
    bool signalled_anything = false;

    struct process *process = find_by_pid(tgid);
    if (!process) {
        return -ESRCH;
    }

    mutex_lock(&process->lock);
    for (struct task *task = process->task_list; task; task = task->process_next) {
        if (task->tid == tid) {
            if (signum != 0) {
                debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(signum));
                task_enqueue_signal(task, signum, NULL, false);

                if (task == get_current_task() && !task_is_sig_blocked(task, signum)) {
                    signalled_self = true;
                }
            }
            signalled_anything = true;
            break;
        }
    }

    mutex_unlock(&process->lock);

    if (signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            sched_run_next();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

int signal_process(pid_t pid, int signum) {
    bool signalled_self = false;
    bool signalled_anything = false;

    struct process *process = find_by_pid(pid);
    if (!process) {
        return -ESRCH;
    }

    mutex_lock(&process->lock);

    // FIXME: dispatch signals to a different task than the first if it makes sense.
    struct task *task = process->task_list;
    if (signum != 0) {
        debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(signum));
        task_enqueue_signal(task, signum, NULL, false);

        if (task == get_current_task() && !task_is_sig_blocked(task, signum)) {
            signalled_self = true;
        }
    }
    signalled_anything = true;

    mutex_unlock(&process->lock);

    if (signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            sched_run_next();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

int queue_signal_process(pid_t pid, int signum, void *val) {
    bool signalled_self = false;
    bool signalled_anything = false;

    struct process *process = find_by_pid(pid);
    if (!process) {
        return -ESRCH;
    }

    mutex_lock(&process->lock);

    // FIXME: dispatch signals to a different task than the first if it makes sense.
    struct task *task = process->task_list;
    if (signum != 0) {
        debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(signum));
        task_enqueue_signal(task, signum, val, true);

        if (task == get_current_task() && !task_is_sig_blocked(task, signum)) {
            signalled_self = true;
        }
    }
    signalled_anything = true;

    mutex_unlock(&process->lock);

    if (signalled_self) {
        struct task *current = get_current_task();
        unsigned long save = disable_interrupts_save();
        task_do_sig(current, signum);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            sched_run_next();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

void exit_process(struct process *process) {
    mutex_lock(&process->lock);
    struct task *task = process->task_list;
    do {
        task_set_state_to_exiting(task);

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
                        wake_user_mutex(um, 1, NULL);
                        *node->__protected = MUTEX_OWNER_DIED;
                        unlock_user_mutex(um);
                    }
                }

                node = node->__next;
            }
        }
    } while ((task = task->process_next));
    mutex_unlock(&process->lock);
}

uint64_t idle_ticks;
uint64_t user_ticks;
uint64_t kernel_ticks;

uint64_t sched_idle_ticks() {
    return idle_ticks;
}

uint64_t sched_user_ticks() {
    return user_ticks;
}

uint64_t sched_kernel_ticks() {
    return kernel_ticks;
}
