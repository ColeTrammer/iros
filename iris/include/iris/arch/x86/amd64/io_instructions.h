#pragma once

#include <di/types/prelude.h>

namespace iris::x86::amd64 {
static inline void io_out(u16 port, di::concepts::OneOf<u8, u16, u32, byte> auto const value) {
    if constexpr (sizeof(value) == 1) {
        asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
    } else if constexpr (sizeof(value) == 2) {
        asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
    } else {
        asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
    }
}

template<di::concepts::OneOf<u8, u16, u32, byte> Out>
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

/// This function is the same as `io_wait()` but it waits for a specified amount of microseconds. Or at least it tries
/// to. This function is a complete hack, since it is not accurate at all. In the future, code should be re-written to
/// use an accurate timer.
static inline void io_wait_us(u32 us) {
    for (u32 i = 0; i < us; i++) {
        io_wait();
    }
}
}
