#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64 {
enum class ModelSpecificRegister : u32 {
    LocalApicBase = 0x1BU,
    Star = 0xC0000081U,
    LStar = 0xC0000082U,
    CStar = 0xC0000083U,
    SfMask = 0xC0000084U,
    FsBase = 0xC0000100U,
    GsBase = 0xC0000101U,
    KernelGsBase = 0xC0000102U,
};

static inline u64 read_msr(ModelSpecificRegister msr) {
    u32 low;
    u32 high;
    asm volatile("rdmsr\n" : "=a"(low), "=d"(high) : "c"(msr));
    return u64(low) | (u64(high) << 32);
}

static inline void write_msr(ModelSpecificRegister msr, u64 value) {
    u32 low = value;
    u32 high = value >> 32;
    asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}
}
