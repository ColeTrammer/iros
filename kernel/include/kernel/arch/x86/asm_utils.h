#ifndef _KERNEL_ARCH_X86_ASM_UTILS_H
#define _KERNEL_ARCH_X86_ASM_UTILS_H 1

#include <assert.h>
#include <limits.h>
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

static inline uint32_t xchg_32(void *ptr, uint32_t x) {
    asm volatile("xchgl %0, %1" : "=r"((uint32_t) x) : "m"(*(volatile uint32_t *) ptr), "0"(x) : "memory");
    return x;
}

static inline void disable_interrupts() {
    asm volatile("cli" : : : "memory");
}

static inline void enable_interrupts() {
    asm volatile("sti" : : : "memory");
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

#define CPUID_FEATURES          1
#define CPUID_EXTENDED_FEATURES 0x80000001

#define CPUID_ECX_RDRAND (1 << 30)

#define CPUID_EDX_1GB_PAGES (1 << 26)

static inline void cpuid(int code, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(b), "=c"(*c), "=d"(*d) : "0"(code));
}

#define MSR_LOCAL_APIC_BASE 0x1BU
#define MSR_STAR            0xC0000081U
#define MSR_LSTAR           0xC0000082U
#define MSR_CSTAR           0xC0000083U
#define MSR_SFMASK          0xC0000084U
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
                 : "r"(msr), "r"((uint32_t) (value & 0xFFFFFFFFU)), "r"((uint32_t) (value >> 32))
                 : "rax", "rcx", "rdx", "memory");
}

static inline uint32_t ilog2(uint32_t x) {
    return sizeof(x) * CHAR_BIT - __builtin_clz(x) - 1;
}

#endif /* _KERNEL_ARCH_X86_ASM_UTILS_H */
