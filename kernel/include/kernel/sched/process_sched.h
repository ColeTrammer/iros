#ifndef _KERNEL_PROCESS_SCHED_H
#define _KERNEL_PROCESS_SCHED_H

#include <kernel/proc/process.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(sched/process_sched.h)

void init_process_sched();
void arch_init_process_sched();

void sched_add_process(struct process *process);
void sched_remove_process(struct process *process);
struct process *find_by_pid(pid_t pid);
void sched_run_next();
void yield_signal();
void yield();

int signal_process_group(pid_t pgid, int signum);
int signal_process(pid_t pid, int signum);

#endif /* _KERNEL_PROCESS_CSHED_H */