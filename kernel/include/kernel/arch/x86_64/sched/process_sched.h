#ifndef _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H
#define _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H 1

#include <kernel/sched/process_sched.h>
#include <kernel/proc/process.h>
#include <kernel/arch/x86_64/proc/process.h>

#define SCHED_CALLBACK_ARG_TYPE struct process_state*

void arch_sched_run_next(struct process_state *process_state);
void sys_sched_run_next(struct process_state *process_state);

#endif /* _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H */