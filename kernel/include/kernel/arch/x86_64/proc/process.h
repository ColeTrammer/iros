#ifndef _KERNEL_ARCH_X86_64_PROC_PROCESS_H
#define _KERNEL_ARCH_X86_64_PROC_PROCESS_H 1

#include <stdint.h>
#include <stdbool.h>

#include <kernel/mem/page.h>
#include <kernel/arch/x86_64/mem/page.h>

#define KERNEL_PROC_STACK_START (((uintptr_t) PT_BASE) - PAGE_SIZE)

struct cpu_state {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
} __attribute__((packed));

struct stack_state {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;    
} __attribute__((packed));

struct process_state {
    struct cpu_state cpu_state;
    struct stack_state stack_state;
} __attribute__((packed));

struct arch_process {
    struct process_state process_state;
    struct process_state user_process_state;
    uint64_t cr3;
    uint64_t kernel_stack;
    struct virt_page_info *kernel_stack_info;
    bool setup_kernel_stack;
};

// Can be longer if more extensions are enabled,
// so this basically needs to be variable length
struct raw_fpu_state {
    uint8_t image[512];
} __attribute__((packed));

struct arch_fpu_state {
    struct raw_fpu_state raw_fpu_state;
    bool saved;
};

extern void __run_process(struct arch_process *state);

#endif /* _KERNEL_ARCH_X86_64_PROC_PROCESS_H */