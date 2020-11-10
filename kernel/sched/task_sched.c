#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/task.h>
#include <kernel/proc/task_finalizer.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>
#include <kernel/util/validators.h>

// #define ROBUST_USER_MUTEX_DEBUG
// #define SCHED_DEBUG
// #define SIGNAL_DEBUG

void init_task_sched() {
    // This only becomes needed after scheduling is enabled
    init_processes();
    init_user_mutexes();
}

static unsigned int next_cpu_id;

void sched_add_task(struct task *task) {
    int target_cpu_id = atomic_fetch_add(&next_cpu_id, 1) % processor_count();

    struct processor *processor = get_processor_list();
    while (processor) {
        if (processor->id == target_cpu_id) {
            if (!processor->enabled) {
                processor = get_processor_list();
                while (processor) {
                    if (processor->enabled) {
                        schedule_task_on_processor(task, processor);
                        return;
                    }
                    processor = processor->next;
                }
                break;
            }

            schedule_task_on_processor(task, processor);
            return;
        }

        processor = processor->next;
    }

    debug_log("All CPUS disabled?");
    assert(false);
}

void local_sched_add_task(struct processor *processor, struct task *task) {
    spin_lock_internal(&processor->sched_lock, __func__, false);
    if (task == processor->current_task) {
        list_append(&processor->sched_list, &task->sched_list);
    } else {
        list_prepend(&processor->sched_list, &task->sched_list);
    }

    if (get_current_processor() != processor && processor->sched_idle) {
        arch_send_ipi(processor);
    }
    spin_unlock(&processor->sched_lock);
}

void local_sched_remove_task(struct processor *processor, struct task *task) {
    spin_lock_internal(&processor->sched_lock, __func__, false);
    list_remove(&task->sched_list);
    spin_unlock(&processor->sched_lock);
}

void sched_maybe_yield(void) {
    struct processor *processor = get_current_processor();
    struct task *current_task = get_current_task();
    if (current_task->sched_ticks_remaining == 0 && processor->preemption_disabled_count == 0) {
        sched_run_next();
    }
}

/* Must be called from unpremptable context */
void sched_run_next() {
    struct processor *processor = get_current_processor();
try_again:
    spin_lock_internal(&processor->sched_lock, __func__, false);
    struct task *to_run = list_first_entry(&processor->sched_list, struct task, sched_list);
    if (to_run) {
        list_remove(&to_run->sched_list);
        list_append(&processor->sched_list, &to_run->sched_list);
        processor->sched_idle = false;
    } else {
        to_run = processor->idle_task;
        processor->sched_idle = true;
    }
    spin_unlock(&processor->sched_lock);

#ifdef SCHED_DEBUG
    if (to_run->process->pid) {
        debug_log("~Running task: [ %d, %d:%d ]\n", processor->id, to_run->tid, to_run->process->pid);
    }
#endif /* SCHED_DEBUG */

    to_run->sched_ticks_remaining = 5;

    assert(to_run->sched_state != WAITING && to_run->sched_state != STOPPED && to_run->sched_state != EXITING);
    int sig;
    spin_lock(&to_run->sig_lock);
    while (to_run->sched_state == RUNNING_INTERRUPTIBLE && (sig = task_get_next_sig(to_run)) != -1) {
        processor->current_task = to_run;
        task_do_sig(to_run, sig);
    }
    spin_unlock(&to_run->sig_lock);

    if (to_run->sched_state != RUNNING_INTERRUPTIBLE && to_run->sched_state != RUNNING_UNINTERRUPTIBLE) {
        goto try_again;
    }
    run_task(to_run);
}

int kernel_yield(void) {
    __kernel_yield();
    return get_current_task()->unblock_result;
}

struct signal_process_group_closure {
    bool signalled_self;
    bool signalled_anything;
    int signum;
};

static void signal_process_group_iter(struct process *process, void *_cls) {
    struct signal_process_group_closure *cls = _cls;

    mutex_lock(&process->lock);

    struct task *task = list_first_entry(&process->task_list, struct task, process_list);
    if (task) {
        if (cls->signum != 0) {
#ifdef SIGNAL_DEBUG
            debug_log("Signaling queue: [ %d:%d, %s ]\n", task->process->pid, task->tid, strsignal(cls->signum));
#endif /* SIGNAL_DEBUG */
            task_enqueue_signal(task, cls->signum, NULL, false);

            if (task == get_current_task() && !task_is_sig_blocked(task, cls->signum)) {
                cls->signalled_self = true;
            }
        }
        cls->signalled_anything = true;
    }

    mutex_unlock(&process->lock);
}

int signal_process_group(pid_t pgid, int signum) {
    struct signal_process_group_closure cls = { .signalled_anything = false, .signalled_self = false, .signum = signum };

    proc_for_each_with_pgid(pgid, signal_process_group_iter, &cls);

    if (cls.signalled_self) {
        unsigned long save = disable_interrupts_save();
        struct task *current = get_current_task();
        spin_lock(&current->sig_lock);
        task_do_sig(current, signum);
        spin_unlock(&current->sig_lock);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            kernel_yield();
        }
        interrupts_restore(save);
    }

    return cls.signalled_anything ? 0 : -ESRCH;
}

struct task *find_by_tid(int tgid, int tid) {
    struct process *process = find_by_pid(tgid);

    if (!process) {
        return NULL;
    }

    mutex_lock(&process->lock);
    list_for_each_entry(&process->task_list, task, struct task, process_list) {
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
    list_for_each_entry(&process->task_list, task, struct task, process_list) {
        if (task->tid == tid) {
            if (signum != 0) {
#ifdef SIGNAL_DEBUG
                debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(signum));
#endif /* SIGNAL_DEBUG */
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
        spin_lock(&current->sig_lock);
        task_do_sig(current, signum);
        spin_unlock(&current->sig_lock);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            kernel_yield();
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
    struct task *task = list_first_entry(&process->task_list, struct task, process_list);
    if (signum != 0) {
#ifdef SIGNAL_DEBUG
        debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(signum));
#endif /* SIGNAL_DEBUG */
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
        spin_lock(&current->sig_lock);
        task_do_sig(current, signum);
        spin_unlock(&current->sig_lock);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            kernel_yield();
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
    struct task *task = list_first_entry(&process->task_list, struct task, process_list);
    if (signum != 0) {
#ifdef SIGNAL_DEBUG
        debug_log("Signaling queue: [ %d, %s ]\n", task->process->pid, strsignal(signum));
#endif /* SIGNAL_DEBUG */
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
        spin_lock(&current->sig_lock);
        task_do_sig(current, signum);
        spin_unlock(&current->sig_lock);
        if (current->sched_state == EXITING || current->sched_state == STOPPED) {
            kernel_yield();
        }
        interrupts_restore(save);
    }

    return signalled_anything ? 0 : -ESRCH;
}

void exit_process(struct process *process, struct task *exclude) {
    list_for_each_entry(&process->task_list, task, struct task, process_list) {
        if (task != exclude) {
            task_set_state_to_exiting(task);
        }

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
    }
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
