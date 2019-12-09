#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

#include <kernel/hal/output.h>
#include <kernel/hal/timer.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/process_state.h>
#include <kernel/proc/task.h>
#include <kernel/proc/user_mutex.h>
#include <kernel/sched/task_sched.h>
#include <kernel/util/spinlock.h>

static struct task *list_start = NULL;
static struct task *list_end = NULL;
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

    // Task signals
    struct task *task = list_start;
    do {
        int sig;
        while ((sig = task_get_next_sig(task)) != -1) {
            task_do_sig(task, sig);
        }
    } while ((task = task->next) != list_start);

    struct task *to_run = current->next;
    while (to_run->sched_state != READY) {
        struct task *next = to_run->next;
        if (to_run->sched_state == EXITING) {
            struct task *to_remove = to_run;

            if (to_remove == list_end) {
                list_end = to_remove->prev;
            }

            if (to_remove == list_start) {
                list_start = to_remove->next;
            }

            struct task *prev_save = to_remove->prev;
            prev_save->next = to_remove->next;
            prev_save->next->prev = prev_save;

            free_task(to_remove, true);
        } else if (to_run->sleeping && to_run->sched_state == WAITING) {
            if (get_time() >= to_run->sleep_end) {
                break;
            }
        } else if (to_run->should_wake_up_from_mutex_sleep && to_run->sched_state == WAITING) {
            to_run->should_wake_up_from_mutex_sleep = false;
            break;
        }

        // Skip taskes that are sleeping
        to_run = next;
    }

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
            task_set_sig_pending(task, signum);
            signalled_anything = true;

            if (task == get_current_task()) {
                signalled_self = true;
            }
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);

    if (signalled_self) {
        yield();
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
            debug_log("Signaling: [ %d, %d ]\n", task->process->pid, signum);
            task_set_sig_pending(task, signum);
            signalled_anything = true;

            if (task == get_current_task()) {
                signalled_self = true;
            }
            break;
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);

    if (signalled_self) {
        yield();
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
            debug_log("Signaling: [ %d, %d ]\n", task->process->pid, signum);
            task_set_sig_pending(task, signum);
            signalled_anything = true;

            if (task == get_current_task()) {
                signalled_self = true;
            }
        }
    } while ((task = task->next) != list_start);

    spin_unlock(&task_list_lock);

    if (signalled_self) {
        yield();
    }

    return signalled_anything ? 0 : -ESRCH;
}

void exit_process(struct process *process) {
    struct task *task = list_start;
    do {
        if (task->process == process) {
            task->sched_state = EXITING;
        }
    } while ((task = task->next) != list_start);
}