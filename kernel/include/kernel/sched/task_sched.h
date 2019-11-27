#ifndef _KERNEL_task_SCHED_H
#define _KERNEL_task_SCHED_H

#include <kernel/proc/task.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(sched/task_sched.h)

void init_task_sched();
void arch_init_task_sched();

void sched_add_task(struct task *task);
void sched_remove_task(struct task *task);
void sched_run_next();
struct task *find_task_for_process(pid_t pid);
void yield_signal();
void yield();

int signal_process_group(pid_t pgid, int signum);
int signal_process(pid_t pid, int signum);

#endif /* _KERNEL_task_SCHED_H */