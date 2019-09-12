#ifndef _KERNEL_PROCESS_SCHED_H
#define _KERNEL_PROCESS_SCHED_H

#include <kernel/proc/process.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(sched/process_sched.h)

void init_process_sched();
void arch_init_process_sched();

void sched_add_process(struct process *process);
void sched_remove_process(struct process *process);
void sched_run_next();

#endif /* _KERNEL_PROCESS_CSHED_H */