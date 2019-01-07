#ifndef _HAL_X86_64_GDT_H
#define _HAL_X86_64_GDT_H 1

#include <stdint.h>

#define GDT_ENTRIES 4

#define CS_OFFSET 1
#define DATA_OFFSET 2
#define CS_SELECTOR (8 * CS_OFFSET)
#define DATA_SELECTOR (8 * DATA_OFFSET)

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

static inline void load_gdt(struct gdt_descriptor descriptor) {
    asm ( "lgdt %0" : : "m"(descriptor) );
}

void init_gdt();

#endif /* _HAL_X86_64_GDT_H */