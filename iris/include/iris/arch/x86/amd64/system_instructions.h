#pragma once

#include <di/prelude.h>
#include <iris/mm/virtual_address.h>

namespace iris::x86::amd64 {
struct [[gnu::packed]] IDTR {
    u16 size;
    u64 virtual_address;
};

static inline void load_idt(IDTR descriptor) {
    asm("lidtq %0" : : "m"(descriptor));
}

struct [[gnu::packed]] GDTR {
    u16 size;
    u64 virtual_address;
};

static inline void load_gdt(GDTR descriptor) {
    asm("lgdt %0" : : "m"(descriptor));
}

static inline void load_tr(u16 selector) {
    asm("ltr %0" : : "m"(selector));
}

static inline u64 read_cr0() {
    u64 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

static inline void load_cr0(u64 cr0) {
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline void load_cr3(u64 cr3) {
    asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

static inline u64 read_cr4() {
    u64 cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

static inline void load_cr4(u64 cr4) {
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}

/// @brief Invalidate the TLB for the provided virtual address.
///
/// @warning In most cases, this function should not be used directly, as it does not invalidate the TLB for all CPUs.
static inline void invlpg(mm::VirtualAddress address) {
    asm volatile("invlpg (%0)" : : "r"(address.raw_value()) : "memory");
}

/// @brief Initalize the floating point state.
static inline void fninit() {
    asm volatile("fninit");
}

/// @brief Save legacy floating point state.
///
/// @warning This requires the provided state is 16 byte-aligned.
static inline void fxsave(di::Byte* state) {
    ASSERT(reinterpret_cast<uptr>(state) % 16 == 0);
    asm volatile("fxsave64 %0" : : "m"(*state));
}

/// @brief Load legacy floating point state.
///
/// @warning This requires the provided state is 16 byte-aligned.
static inline void fxrstor(di::Byte* state) {
    ASSERT(reinterpret_cast<uptr>(state) % 16 == 0);
    asm volatile("fxrstor64 %0" : : "m"(*state));
}

/// @brief Load extended floating point state.
///
/// @warning This requires that the CPU support has been detecetd, and that the provided state is 64 byte-aligned.
static inline void xsave(di::Byte* state) {
    ASSERT(reinterpret_cast<uptr>(state) % 64 == 0);
    asm volatile("xsave %0" ::"m"(*state));
}

/// @brief Save extended floating point state.
///
/// @warning This requires that the CPU support has been detecetd, and that the provided state is 64 byte-aligned.
static inline void xrstor(di::Byte* state) {
    ASSERT(reinterpret_cast<uptr>(state) % 64 == 0);
    asm volatile("xrstor %0" : : "m"(*state));
}

/// @brief Set extended control register.
///
/// @warning This requires CPU support has been detected.
static inline void xsetbv(u32 reg, u64 value) {
    auto low = u32(value);
    auto high = u32(value >> 32);
    asm volatile("xsetbv" : : "a"(low), "c"(reg), "d"(high));
}

/// @brief Set the fs base register value.
///
/// @warning Calling this function requires that CPU support has been detected and enabled.
static inline void write_fs_base(u64 value) {
    asm volatile("wrfsbase %0" : : "r"(value));
}

/// @brief Set the gs base register value.
///
/// @warning Calling this function requires that CPU support has been detected and enabled.
static inline void write_gs_base(u64 value) {
    asm volatile("wrgsbase %0" : : "r"(value));
}

/// @brief Swap gs base register value.
///
/// @warning This function is called to load the current processor on entry from the `syscall` instruction or an
/// interrupt. Incorrect usage will cause the kernel to crash after reading garbage values.
static inline void swapgs() {
    asm volatile("swapgs" ::: "memory");
}
}
