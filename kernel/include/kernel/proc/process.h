#ifndef _KERNEL_PROC_PROCESS_H
#define _KERNEL_PROC_PROCESS_H 1

#include <stdbool.h>
#include <sys/types.h>

#include <kernel/mem/vm_region.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(proc/process.h)

struct process {
    struct arch_process arch_process;
    struct vm_region *process_memory;
    bool kernel_process;
    pid_t pid;
    struct process *next;
};

#endif /* _KERNEL_PROC_PROCESS_H */