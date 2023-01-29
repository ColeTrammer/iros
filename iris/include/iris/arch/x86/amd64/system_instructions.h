#pragma once

#include <di/prelude.h>

namespace iris::arch::amd64 {
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

static inline void load_cr3(u64 cr3) {
    asm volatile("mov %0, %%rdx\n"
                 "mov %%rdx, %%cr3\n"
                 :
                 : "m"(cr3)
                 : "rdx", "memory");
}
}