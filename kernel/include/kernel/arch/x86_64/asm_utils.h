#ifndef _KERNEL_ARCH_X86_64_ASM_UTILS_H
#define _KERNEL_ARCH_X86_64_ASM_UTILS_H 1

#include <stdint.h>

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

static inline void io_wait() {
    asm volatile ( "outb %%al, $0x80" : : "a"(0) );
}

static inline void invlpg(uintptr_t addr) {
    asm volatile( "invlpg (%0)" : : "b"(addr) : "memory" );
}

static inline void load_cr3(uintptr_t cr3) {
    asm( "mov %0, %%rdx\n"\
         "mov %%rdx, %%cr3\n"
         : : "m"(cr3) : "rdx" );
}

static inline uint64_t get_cr3() {
    uint64_t cr3;
    asm( "mov %%cr3, %%rdx\n"\
         "mov %%rdx, %0" : "=m"(cr3) : : "rdx" );
    return cr3;
}

static inline uint64_t get_rflags() {
    uint64_t rflags;
    asm ( "pushfq\n"\
          "popq %%rdx\n"\
          "mov %%rdx, %0" : "=m"(rflags) : : "rdx" );
    return rflags;
}

static inline uint32_t xchg_32(void *ptr, uint32_t x)
{
	asm volatile( "xchgl %0, %1" : "=r" ((uint32_t) x) : "m" (*(volatile uint32_t*) ptr), "0"(x) : "memory" );
	return x;
}

#endif