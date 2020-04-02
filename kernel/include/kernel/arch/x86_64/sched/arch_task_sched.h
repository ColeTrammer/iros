#ifndef _KERNEL_ARCH_X86_64_SCHED_ARCH_TASK_SCHED_H
#define _KERNEL_ARCH_X86_64_SCHED_ARCH_TASK_SCHED_H 1

#include <kernel/proc/task.h>

#define SCHED_CALLBACK_ARG_TYPE struct task_state *

void arch_sched_run_next(struct task_state *task_state);
void sys_sched_run_next(struct task_state *task_state);

#endif /* _KERNEL_ARCH_X86_64_SCHED_ARCH_TASK_SCHED_H */