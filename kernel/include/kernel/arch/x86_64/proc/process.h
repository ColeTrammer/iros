#ifndef _KERNEL_ARCH_X86_64_PROC_PROCESS_H
#define _KERNEL_ARCH_X86_64_PROC_PROCESS_H 1

#include <stdint.h>

struct cpu_state {

} __attribute__((packed));

struct stack_state {
    
} __attribute__((packed));

struct process_state {
    struct cpu_state cpu_state;
    struct stack_state stack_state;
} __attribute__((packed));

struct arch_process {
    struct process_state process_state;
    uint64_t cr3;
};

#endif /* _KERNEL_ARCH_X86_64_PROC_PROCESS_H */