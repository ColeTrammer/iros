#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>

#include <kernel/mem/vm_region.h>
#include <kernel/fs/file.h>

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
    enum sched_state sched_state;
    
    char *cwd;
    struct file *files[FOPEN_MAX];

    struct process *prev;
    struct process *next;

    struct arch_fpu_state fpu;
};

void init_kernel_process();
void arch_init_kernel_process(struct process *kernel_process);

struct process *load_process(const char *file_name);
void arch_load_process(struct process *process, uintptr_t entry);

void run_process(struct process *process);
void arch_run_process(struct process *process);

void free_process(struct process *process, bool free_paging_structure, bool free_pid);
void arch_free_process(struct process *process, bool free_paging_structure);

struct process *get_current_process();

uintptr_t map_program_args(uintptr_t start, char **argv, char **envp);

#endif /* _KERNEL_PROC_PROCESS_H */