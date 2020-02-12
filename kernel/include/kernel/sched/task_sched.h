#ifndef _KERNEL_task_SCHED_H
#define _KERNEL_task_SCHED_H

#include <kernel/proc/task.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(sched/task_sched.h)
// clang-format on

void init_task_sched();
void arch_init_task_sched();

struct task *find_by_tid(int tgid, int tid);

void sched_add_task(struct task *task);
void sched_remove_task(struct task *task);
void sched_run_next();
struct task *find_task_for_process(pid_t pid);
void yield_signal();
void __kernel_yield();

int signal_task(int tgid, int tid, int signum);
int signal_process_group(pid_t pgid, int signum);
int signal_process(pid_t pid, int signum);
int queue_signal_process(pid_t pid, int signum, void *val);

void exit_process(struct process *process);

#endif /* _KERNEL_task_SCHED_H */