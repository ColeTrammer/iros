#ifndef _KERNEL_HAL_X86_64_GDT_H
#define _KERNEL_HAL_X86_64_GDT_H 1

#include <stdint.h>

#define GDT_ENTRIES 7

#define CS_OFFSET        1
#define DATA_OFFSET      2
#define USER_CODE_OFFSET 3
#define USER_DATA_OFFSET 4
#define TSS_OFFSET       5

#define CS_SELECTOR        (CS_OFFSET * 8)
#define DATA_SELECTOR      (DATA_OFFSET * 8)
#define USER_CODE_SELECTOR ((USER_CODE_OFFSET * 8) | 0b11)
#define USER_DATA_SELECTOR ((USER_DATA_OFFSET * 8) | 0b11)
#define TSS_SELECTOR       (TSS_OFFSET * 8)

#define TSS_TYPE 0b10001001;

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
    uint32_t base_high;
    uint16_t zero;
} __attribute__((packed));

struct tss {
    uint32_t reserved1;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved2;
    uint64_t ist[7];
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t io_map_base;
} __attribute__((packed));

static inline void load_gdt(struct gdt_descriptor descriptor) {
    asm("lgdt %0" : : "m"(descriptor));
}

static inline void load_tr(uint16_t selector) {
    asm("ltr %0" : : "m"(selector));
}

void init_gdt();
void set_tss_stack_pointer(uintptr_t rsp);

#endif /* _KERNEL_HAL_X86_64_GDT_H */