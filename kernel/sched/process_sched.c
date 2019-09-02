#include <stddef.h>

#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>

static struct process *list_start = NULL;
static struct process *list_end = NULL;

void init_process_sched() {

}

void sched_add_process(struct process *process) {
    if (list_start == NULL) {
        list_start = list_end = process;
        return;
    }

    list_end->next = process;
    process->next = list_start;
    list_end = process;
}

void sched_run_next() {
    run_process(get_current_process()->next);
}

