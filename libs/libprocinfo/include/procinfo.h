#ifndef _PROCINFO_H
#define _PROCINFO_H 1

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#define READ_PROCFS_SCHED 1

#define READ_PROCFS_GLOBAL_SCHED   1
#define READ_PROCFS_GLOBAL_MEMINFO 2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct proc_info {
    char name[64];
    pid_t pid;
    char state[64];
    uid_t uid;
    gid_t gid;
    pid_t ppid;
    mode_t umask;
    uid_t euid;
    gid_t egid;
    pid_t pgid;
    pid_t sid;
    char tty[64];
    int priority;
    int nice;
    size_t virtual_memory;
    size_t resident_memory;
    struct timespec start_time;
    uint64_t user_ticks;
    uint64_t kernel_ticks;
};

struct proc_global_info {
    uintptr_t total_memory;
    uint64_t idle_ticks;
    uint64_t user_ticks;
    uint64_t kernel_ticks;
};

int read_procfs_info(struct proc_info **info, size_t *length, int flags);
void free_procfs_info(struct proc_info *info);
int read_procfs_global_info(struct proc_global_info *info, int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PROCINFO_H */