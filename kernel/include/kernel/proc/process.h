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

void load_process(const char *file_name);
void run_process(uint64_t rip, uint64_t rsp);

#endif /* _KERNEL_PROC_PROCESS_H */