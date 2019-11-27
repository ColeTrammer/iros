#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <signal.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/util/spinlock.h>

struct process {
    char *cwd;
    struct file *files[FOPEN_MAX];

    struct vm_region *process_memory;

    pid_t pid;
    pid_t pgid;
    pid_t ppid;

    ino_t inode_id;
    dev_t inode_dev;

    int tty;
    int ref_count;

    struct tms times;

    struct sigaction sig_state[_NSIG];
    spinlock_t lock;
};

void proc_drop_process_unlocked(struct process *process);
void proc_drop_process(struct process *process);
void proc_add_process(struct process *process);
struct process *find_by_pid(pid_t pid);
void proc_set_sig_pending(struct process *process, int n);
void init_processes();

#endif /* _KERNEL_PROC_PROCESS_H */