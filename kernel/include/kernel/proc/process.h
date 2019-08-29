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

struct process *load_process(const char *file_name);
void arch_load_process(struct process *process, uintptr_t entry);

void run_process(struct process *process);
void arch_run_process(struct process *process);

static inline uint64_t get_rflags() {
    uint64_t rflags;
    asm ( "pushfq\n"\
          "popq %%rdx\n"\
          "mov %%rdx, %0" : "=m"(rflags) : : "rdx" );
    return rflags;
}

#endif /* _KERNEL_PROC_PROCESS_H */