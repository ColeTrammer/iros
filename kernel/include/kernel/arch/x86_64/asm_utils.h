#ifndef _KERNEL_ARCH_X86_64_ASM_UTILS_H
#define _KERNEL_ARCH_X86_64_ASM_UTILS_H 1

#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>

#define barrier()   asm volatile("" : : : "memory")
#define cpu_relax() asm volatile("pause" : : : "memory")

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait() {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

static inline void io_wait_us(uint32_t micro_seconds) {
    for (uint32_t i = 0; i < micro_seconds; i++) {
        io_wait();
    }
}

static inline void invlpg(uintptr_t addr) {
    asm volatile("invlpg (%0)" : : "b"(addr) : "memory");
}

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

static inline uint32_t xchg_32(void *ptr, uint32_t x) {
    asm volatile("xchgl %0, %1" : "=r"((uint32_t) x) : "m"(*(volatile uint32_t *) ptr), "0"(x) : "memory");
    return x;
}

static inline void disable_interrupts() {
    asm volatile("cli" : : : "memory");
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

static inline void enable_interrupts() {
    asm volatile("sti" : : : "memory");
}

static inline void fxsave(uint8_t *state) {
    assert((uintptr_t) state % 16 == 0);
    asm volatile("fxsave64 %0" : : "m"(*state) : "memory");
}

static inline void fxrstor(uint8_t *state) {
    assert((uintptr_t) state % 16 == 0);
    asm volatile("fxrstor64 %0" : : "m"(*state) : "memory");
}

static inline void fninit(void) {
    asm volatile("fninit" : : : "memory");
}

static inline uint32_t get_mxcsr(void) {
    uint32_t mxcsr;
    asm volatile("stmxcsr %0" : "=m"(mxcsr) : : "memory");
    return mxcsr;
}

static inline void set_mxcsr(uint32_t mxcsr) {
    asm volatile("ldmxcsr %0" : : "m"(mxcsr) : "memory");
}

static inline void swapgs(void) {
    asm volatile("swapgs" : : : "memory");
}

#define CPUID_FEATURES 1

#define CPUID_ECX_RDRAND (1 << 30)

static inline void cpuid(int code, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(b), "=c"(*c), "=d"(*d) : "0"(code));
}

#define MSR_LOCAL_APIC_BASE 0x1BU
#define MSR_FS_BASE         0xC0000100U
#define MSR_GS_BASE         0xC0000101U
#define MSR_KERNEL_GS_BASE  0xC0000102U

static inline uint64_t get_msr(uint32_t msr) {
    uint32_t low;
    uint32_t high;
    asm volatile("movl %2, %%ecx\n"
                 "rdmsr\n"
                 "movl %%eax, %0\n"
                 "movl %%edx, %1\n"
                 : "=r"(low), "=r"(high)
                 : "r"(msr)
                 : "rax", "rcx", "rdx", "memory");
    return ((uint64_t) low) | ((uint64_t) high) << 32ULL;
}

static inline void set_msr(uint32_t msr, uint64_t value) {
    asm volatile("movl %0, %%ecx\n"
                 "movl %1, %%eax\n"
                 "movl %2, %%edx\n"
                 "wrmsr"
                 :
                 : "r"(msr), "r"((uint32_t)(value & 0xFFFFFFFFU)), "r"((uint32_t)(value >> 32))
                 : "rax", "rcx", "rdx", "memory");
}

#endif /* _KERNEL_ARCH_X86_64_ASM_UTILS_H */
