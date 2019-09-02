#ifndef _KERNEL_PROCESS_SCHED_H
#define _KERNEL_PROCESS_SCHED_H

#include <kernel/proc/process.h>

void init_process_sched();

void sched_add_process(struct process *process);
void sched_run_next();

#endif /* _KERNEL_PROCESS_CSHED_H */