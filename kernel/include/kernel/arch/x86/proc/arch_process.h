#ifndef _KERNEL_ARCH_X86_PROC_ARCH_PROCESS_H
#define _KERNEL_ARCH_X86_PROC_ARCH_PROCESS_H 1

#include <stdint.h>

struct arch_process {
    uint64_t cr3;
};

struct process;

void proc_kill_arch_process(struct process *process, bool free_paging_structure);

#endif /* _KERNEL_ARCH_X86_PROC_ARCH_PROCESS_H */
