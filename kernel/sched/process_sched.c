#include <stddef.h>

#include <kernel/hal/output.h>

#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>

static struct process *list_start = NULL;
static struct process *list_end = NULL;

void init_process_sched() {
    arch_init_process_sched();
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

    struct process *p = list_start;
    do {
        debug_log("Process: [ %d, %#.16lX, %#.16lX, %#.16lX ]\n", p->pid, p, p->prev, p->next);
        p = p->next;
    } while (p != list_start);
}

void sched_run_next() {
    struct process *current = get_current_process();

    while (current->next->sched_state == EXITING) {
        struct process *to_remove = current->next;

        current->next = current->next->next;
        current->next->prev = current;

        free_process(to_remove);
    }

    if (current == current->next) {
        while (1);
    }

    run_process(current->next);
}

