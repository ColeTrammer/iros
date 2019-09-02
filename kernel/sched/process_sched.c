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
        process->prev = process->next = process;
        return;
    }

    process->prev = list_end;
    process->next = list_start;

    list_end->next = process;
    list_start->prev = process;
    list_end = process;
}

void sched_run_next() {
    struct process *current = get_current_process();
    struct process *next = current->next;
    if (current->sched_state == EXITING) {
        current->prev->next = current->next;
        current->next->prev = current->prev;
        
        // free_process(current);
    }

    if (current->next == current) {
        while (1);
    }

    run_process(next);
}

