#ifndef _KERNEL_ARCH_X86_64_ASM_UTILS_H
#define _KERNEL_ARCH_X86_64_ASM_UTILS_H 1

#include <stdint.h>
#include <stdatomic.h>

#define barrier() asm volatile( "" : : : "memory" )
#define cpu_relax() asm volatile( "pause" : : : "memory" )

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void io_wait() {
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

static inline void invlpg(uintptr_t addr) {
    asm volatile( "invlpg (%0)" : : "b"(addr) : "memory" );
}

static inline void load_cr3(uintptr_t cr3) {
    asm volatile( "mov %0, %%rdx\n"\
                  "mov %%rdx, %%cr3\n"
                  : : "m"(cr3) : "rdx" );
}

static inline uint64_t get_cr3() {
    uint64_t cr3;
    asm volatile( "mov %%cr3, %%rdx\n"\
                  "mov %%rdx, %0" : "=m"(cr3) : : "rdx" );
    return cr3;
}

static inline uint64_t get_rflags() {
    uint64_t rflags;
    asm volatile( "pushfq\n"\
                  "popq %0\n" : "=r"(rflags) : : "memory" );
    return rflags;
}

static inline void set_rflags(uint64_t rflags) {
    asm volatile( "pushq %0\n"\
                  "popfq" : : "rm"(rflags) : "memory", "cc" );
}

static inline uint32_t xchg_32(void *ptr, uint32_t x)
{
	asm volatile( "xchgl %0, %1" : "=r" ((uint32_t) x) : "m" (*(volatile uint32_t*) ptr), "0"(x) : "memory" );
	return x;
}

static inline void disable_interrupts() {
    asm volatile ( "cli" : : : "memory" );
}

static inline unsigned long disable_interrupts_save() {
    unsigned long flags;
    asm volatile( "pushfq\n"\
                  "cli\n"\
                  "popq %0" : "=r"(flags) : : "memory" );
    return flags;
}

static inline void interrupts_restore(unsigned long flags) {
    asm volatile( "pushq %0\n"\
                  "popfq" : : "rm"(flags) : "memory", "cc" );
}

static inline void enable_interrupts() {
    asm volatile ( "sti"  : : : "memory" );
}

#endif