#include <stddef.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>

#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/process.h>
#include <kernel/proc/process_state.h>
#include <kernel/hal/timer.h>
#include <kernel/sched/process_sched.h>
#include <kernel/util/spinlock.h>

static struct process *list_start = NULL;
static struct process *list_end = NULL;
static spinlock_t process_list_lock = SPINLOCK_INITIALIZER;

void init_process_sched() {
    // This only becomes needed after scheduling is enabled
    init_proc_state();

    arch_init_process_sched();
}

void sched_add_process(struct process *process) {
    spin_lock(&process_list_lock);

    if (list_start == NULL) {
        list_start = list_end = process;
        process->prev = process->next = process;
        
        spin_unlock(&process_list_lock);
        return;
    }

    process->prev = list_end;
    process->next = list_start;

    list_end->next = process;
    list_start->prev = process;
    list_end = process;

    spin_unlock(&process_list_lock);
}

void sched_remove_process(struct process *process) {
    spin_lock(&process_list_lock);

    if (list_start == NULL) {        
        spin_unlock(&process_list_lock);
        return;
    }

    struct process *current = list_start;

    while (current->next != process) {
        current = current->next;
    }

    if (process == list_end) {
        list_end = process->prev;
    }

    if (process == list_start) {
        list_start = process->next;
    }

    current->next = current->next->next;
    current->next->prev = current;

    spin_unlock(&process_list_lock);
}

struct process *find_by_pid(pid_t pid) {
    /* Not SMP Safe Because It Can't Lock The Process List */

    struct process *p = list_start;
    do {
        if (p->pid == pid) {
            return p;
        }

        p = p->next;
    } while (p != list_start);

    return NULL;
}

/* Must be called from unpremptable context */
void sched_run_next() {
    struct process *current = get_current_process();

    // Process signals
    struct process *process = list_start;
    do {
        int sig;
        while ((sig = proc_get_next_sig(process)) != -1) {
            proc_do_sig(process, sig);
        }
    } while ((process = process->next) != list_start);

    struct process *to_run = current->next;
    while (to_run->sched_state != READY) {
        if (to_run->sched_state == EXITING) {
            struct process *to_remove = to_run;

            if (to_remove == list_end) {
                list_end = to_remove->prev;
            }

            if (to_remove == list_start) {
                list_start = to_remove->next;
            }

            struct process *prev_save = to_remove->prev;
            prev_save->next = to_remove->next;
            prev_save->next->prev = prev_save;

            free_process(to_remove, true);
        } else if (to_run->sleeping && to_run->sched_state == WAITING) {
            if (get_time() >= to_run->sleep_end) {
                break;
            }
        }

        // Skip processes that are sleeping
        to_run = to_run->next;
    }

    run_process(to_run);
}

int signal_process_group(pid_t pgid, int signum) {
    spin_lock(&process_list_lock);

    bool signalled_self = false;
    bool signalled_anything = false;
    struct process *process = list_start;
    do {
        if (process->pgid == pgid) {
            proc_set_sig_pending(process, signum);
            signalled_anything = true;

            if (process == get_current_process()) {
                signalled_self = true;
            }
        } 
    } while ((process = process->next) != list_start);

    spin_unlock(&process_list_lock);

    if (signalled_self) {
        yield();
    }

    return signalled_anything ? 0 : -ESRCH;
}

int signal_process(pid_t pid, int signum) {
    spin_lock(&process_list_lock);

    bool signalled_self = false;
    bool signalled_anything = false;
    struct process *process = list_start;
    do {
        // Maybe should only do it once instead of in a loop
        if (process->pid == pid) {
            debug_log("Signaling: [ %d, %d ]\n", process->pid, signum);
            proc_set_sig_pending(process, signum);
            signalled_anything = true;

            if (process == get_current_process()) {
                signalled_self = true;
            }
        } 
    } while ((process = process->next) != list_start);

    spin_unlock(&process_list_lock);

    if (signalled_self) {
        yield();
    }

    return signalled_anything ? 0 : -ESRCH;
}