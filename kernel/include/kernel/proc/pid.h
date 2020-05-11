#ifndef _KERNEL_PROC_PID_H
#define _KERNEL_PROC_PID_H 1

#include <sys/types.h>

pid_t get_next_pid();
void free_pid(pid_t pid);

#endif /* _KERNEL_PROC_PID_H */
