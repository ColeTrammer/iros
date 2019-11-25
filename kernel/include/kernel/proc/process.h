#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>

#include <kernel/fs/file.h>
#include <kernel/mem/vm_region.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(proc/process.h)

enum sched_state {
    RUNNING,
    READY,
    WAITING,
    EXITING
};

struct process {
    struct arch_process arch_process;

    struct vm_region *process_memory;
    bool kernel_process;

    pid_t pid;
    pid_t pgid;
    pid_t ppid;

    enum sched_state sched_state;
    
    char *cwd;
    struct file *files[FOPEN_MAX];
    bool in_kernel;
    bool in_sigsuspend;
    bool can_send_self_signals;

    struct process *prev;
    struct process *next;

    struct sigaction sig_state[_NSIG];
    sigset_t sig_mask;
    sigset_t saved_sig_mask;
    sigset_t sig_pending;

    bool sleeping;
    time_t sleep_end;

    ino_t inode_id;
    dev_t inode_dev;

    int tty;

    struct tms times;

    struct arch_fpu_state fpu;
};

void init_kernel_process();
void arch_init_kernel_process(struct process *kernel_process);

struct process *load_kernel_process(uintptr_t entry);
void arch_load_kernel_process(struct process *process, uintptr_t entry);

struct process *load_process(const char *file_name);
void arch_load_process(struct process *process, uintptr_t entry);

void run_process(struct process *process);
void arch_run_process(struct process *process);

void free_process(struct process *process, bool free_paging_structure);
void arch_free_process(struct process *process, bool free_paging_structure);

struct process *get_current_process();

uintptr_t map_program_args(uintptr_t start, char **argv, char **envp);

void proc_set_sig_pending(struct process *process, int signum);
void proc_unset_sig_pending(struct process *process, int signum);
int proc_get_next_sig(struct process *process);
void proc_do_sig(struct process *process, int signum);
void proc_do_sig_handler(struct process *process, int signum);
bool proc_is_sig_blocked(struct process *process, int signum);
void proc_notify_parent(pid_t child_pid);

bool proc_in_kernel(struct process *process);

#endif /* _KERNEL_PROC_PROCESS_H */