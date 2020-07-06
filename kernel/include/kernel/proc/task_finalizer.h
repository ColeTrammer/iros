#ifndef _KERNEL_PROC_TASK_FINALIZER_H
#define _KERNEL_PROC_TASK_FINALIZER_H 1

struct task;

void proc_schedule_task_for_destruction(struct task *task);

void init_task_finalizer(void);

#endif /* _KERNEL_PROC_TASK_FINALIZER_H */
