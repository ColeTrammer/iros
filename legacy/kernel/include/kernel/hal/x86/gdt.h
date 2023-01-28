#ifndef _KERNEL_HAL_X86_GDT_H
#define _KERNEL_HAL_X86_GDT_H 1

#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t type;
    uint8_t limit_high : 4;
    uint8_t flags : 4;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_descriptor {
    uint16_t limit;
    struct gdt_entry *gdt;
} __attribute__((packed));

struct gdt_tss_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_low_mid;
    uint8_t type;
    uint8_t limit_high : 4;
    uint8_t flags : 4;
    uint8_t base_high_mid;
#ifdef __x86_64__
    uint32_t base_high;
    uint16_t zero;
#endif /* __x86_64__ */
} __attribute__((packed));

static inline void load_gdt(struct gdt_descriptor descriptor) {
    asm("lgdt %0" : : "m"(descriptor));
}

static inline void load_tr(uint16_t selector) {
    asm("ltr %0" : : "m"(selector));
}

#endif /* _KERNEL_HAL_X86_GDT_H */
