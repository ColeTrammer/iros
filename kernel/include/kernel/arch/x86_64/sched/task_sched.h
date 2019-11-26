#ifndef _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H
#define _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H 1

#include <kernel/sched/task_sched.h>
#include <kernel/proc/task.h>
#include <kernel/arch/x86_64/proc/task.h>

#define SCHED_CALLBACK_ARG_TYPE struct task_state*

void arch_sched_run_next(struct task_state *task_state);
void sys_sched_run_next(struct task_state *task_state);

#endif /* _KERNEL_ARCH_X86_64_SCHED_PROCESS_SCHED_H */