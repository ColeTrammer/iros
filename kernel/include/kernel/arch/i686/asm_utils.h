#ifndef _KERNEL_ARCH_I386_ASM_UTILS_H
#define _KERNEL_ARCH_I386_ASM_UTILS_H 1

#include <kernel/arch/x86/asm_utils.h>

static inline void load_cr3(uintptr_t cr3) {
    asm volatile("mov %0, %%edx\n"
                 "mov %%edx, %%cr3\n"
                 :
                 : "m"(cr3)
                 : "edx");
}

static inline uint32_t get_cr2() {
    uint32_t cr2;
    asm volatile("mov %%cr2, %%edx\n"
                 "mov %%edx, %0"
                 : "=m"(cr2)
                 :
                 : "edx");
    return cr2;
}

static inline uint32_t get_cr3() {
    uint32_t cr3;
    asm volatile("mov %%cr3, %%edx\n"
                 "mov %%edx, %0"
                 : "=m"(cr3)
                 :
                 : "edx");
    return cr3;
}

static inline uint64_t get_base_pointer() {
    uint32_t ebp;
    asm volatile("mov %%ebp, %0" : "=a"(ebp) : :);
    return ebp;
}

static inline uint32_t get_rflags() {
    uint32_t rflags;
    asm volatile("pushf\n"
                 "pop %0\n"
                 : "=r"(rflags)
                 :
                 : "memory");
    return rflags;
}

static inline void set_rflags(uint32_t rflags) {
    asm volatile("pushq %0\n"
                 "popfq"
                 :
                 : "rm"(rflags)
                 : "memory", "cc");
}

static inline unsigned long disable_interrupts_save() {
    unsigned long flags;
    asm volatile("pushf\n"
                 "cli\n"
                 "pop %0"
                 : "=r"(flags)
                 :
                 : "memory");
    return flags;
}

static inline void interrupts_restore(unsigned long flags) {
    asm volatile("push %0\n"
                 "popf"
                 :
                 : "rm"(flags)
                 : "memory", "cc");
}

static inline void fxsave(uint8_t* state) {
    assert((uintptr_t) state % 16 == 0);
    asm volatile("fxsave %0" : : "m"(*state) : "memory");
}

static inline void fxrstor(uint8_t* state) {
    assert((uintptr_t) state % 16 == 0);
    asm volatile("fxrstor %0" : : "m"(*state) : "memory");
}

#endif /* _KERNEL_ARCH_I386_ASM_UTILS_H */
