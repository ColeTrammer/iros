#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64 {
static inline void io_out(u16 port, di::concepts::OneOf<u8, u16, u32> auto const value) {
    if constexpr (sizeof(value) == 1) {
        asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
    } else if constexpr (sizeof(value) == 2) {
        asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
    } else {
        asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
    }
}

template<di::concepts::OneOf<u8, u16, u32> Out>
static inline Out io_in(u16 port) {
    Out value;
    if constexpr (sizeof(Out) == 1) {
        asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    } else if constexpr (sizeof(Out) == 2) {
        asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    } else {
        asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    }
    return value;
}

/// This functions writes to a unused IO port to implement a small delay.
/// This is mostly used for old hardware the CPU needs to wait for to properly
/// function. Modern devices use MMIO and obsolete the IO ports.
static inline void io_wait() {
    // NOTE: OSDEV says that port 0x80 will not be used after boot, citing Linux.
    //       https://wiki.osdev.org/Inline_Assembly/Examples#IO_WAIT
    io_out(0x80, (u8) 0);
}
}