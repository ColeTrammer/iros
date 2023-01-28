#ifndef _KERNEL_ARCH_X86_64_ASM_UTILS_H
#define _KERNEL_ARCH_X86_64_ASM_UTILS_H 1

#include <kernel/arch/x86/asm_utils.h>

static inline void load_cr3(uintptr_t cr3) {
    asm volatile("mov %0, %%rdx\n"
                 "mov %%rdx, %%cr3\n"
                 :
                 : "m"(cr3)
                 : "rdx");
}

static inline uint64_t get_cr2() {
    uint64_t cr2;
    asm volatile("mov %%cr2, %%rdx\n"
                 "mov %%rdx, %0"
                 : "=m"(cr2)
                 :
                 : "rdx");
    return cr2;
}

static inline uint64_t get_cr3() {
    uint64_t cr3;
    asm volatile("mov %%cr3, %%rdx\n"
                 "mov %%rdx, %0"
                 : "=m"(cr3)
                 :
                 : "rdx");
    return cr3;
}

static inline uint64_t get_base_pointer() {
    uint64_t rbp;
    asm volatile("mov %%rbp, %0" : "=a"(rbp) : :);
    return rbp;
}

static inline uint64_t get_stack_pointer() {
    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=a"(rsp) : :);
    return rsp;
}

static inline uint64_t get_rflags() {
    uint64_t rflags;
    asm volatile("pushfq\n"
                 "popq %0\n"
                 : "=r"(rflags)
                 :
                 : "memory");
    return rflags;
}

static inline void set_rflags(uint64_t rflags) {
    asm volatile("pushq %0\n"
                 "popfq"
                 :
                 : "rm"(rflags)
                 : "memory", "cc");
}

static inline unsigned long disable_interrupts_save() {
    unsigned long flags;
    asm volatile("pushfq\n"
                 "cli\n"
                 "popq %0"
                 : "=r"(flags)
                 :
                 : "memory");
    return flags;
}

static inline void interrupts_restore(unsigned long flags) {
    asm volatile("pushq %0\n"
                 "popfq"
                 :
                 : "rm"(flags)
                 : "memory", "cc");
}

static inline void fxsave(uint8_t* state) {
    assert((uintptr_t) state % 16 == 0);
    asm volatile("fxsave64 %0" : : "m"(*state) : "memory");
}

static inline void fxrstor(uint8_t* state) {
    assert((uintptr_t) state % 16 == 0);
    asm volatile("fxrstor64 %0" : : "m"(*state) : "memory");
}

#endif /* _KERNEL_ARCH_X86_64_ASM_UTILS_H */
