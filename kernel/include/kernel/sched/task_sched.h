#ifndef _KERNEL_task_SCHED_H
#define _KERNEL_task_SCHED_H

struct processor;
struct process;
struct task_state;

void init_task_sched(void);

struct task *find_by_tid(int tgid, int tid);

void sched_add_task(struct task *task);

void sched_tick(struct task_state *task_state);

void local_sched_add_task(struct processor *processor, struct task *task);
void local_sched_remove_task(struct processor *processor, struct task *task);
void sched_run_next(void);
void arch_sched_run_next(struct task_state *task_state);
void __kernel_yield(void);
int kernel_yield(void);

int signal_task(int tgid, int tid, int signum);
int signal_process_group(pid_t pgid, int signum);
int signal_process(pid_t pid, int signum);
int queue_signal_process(pid_t pid, int signum, void *val);

void exit_process(struct process *process, struct task *exclude);

uint64_t sched_idle_ticks(void);
uint64_t sched_user_ticks(void);
uint64_t sched_kernel_ticks(void);

#endif /* _KERNEL_task_SCHED_H */
