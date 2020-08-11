#ifndef _KERNEL_PROC_PROFILE_H
#define _KERNEL_PROC_PROFILE_H 1

#include <stdint.h>
#include <sys/os_2.h>
#include <sys/types.h>

struct process;
struct task_state;

void proc_record_profile_stack(struct task_state *task_state);
void proc_record_memory_map(struct process *process);
void proc_write_profile_buffer(struct process *process, const void *buffer, size_t size);

int proc_enable_profiling(pid_t pid);
ssize_t proc_read_profile(pid_t pid, void *buffer, size_t size);
int proc_disable_profiling(pid_t pid);

#endif /* _KERNEL_PROC_PROFILE_H */
