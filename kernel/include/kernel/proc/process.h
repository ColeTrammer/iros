#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <signal.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/util/spinlock.h>

// clang-format off
#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(proc/process.h)
// clang-format on

struct process {
    char *cwd;
    struct file *files[FOPEN_MAX];

    struct vm_region *process_memory;

    pid_t pid;
    pid_t pgid;
    pid_t ppid;

    int tty;
    int ref_count;

    struct arch_process arch_process;

    ino_t inode_id;
    dev_t inode_dev;

    struct tms times;

    struct sigaction sig_state[_NSIG];
    spinlock_t lock;
};

void proc_drop_process_unlocked(struct process *process, bool free_paging_structure);
void proc_drop_process(struct process *process, bool free_paging_structure);
void proc_add_process(struct process *process);
struct process *find_by_pid(pid_t pid);
void proc_set_sig_pending(struct process *process, int n);
void init_processes();

#endif /* _KERNEL_PROC_PROCESS_H */