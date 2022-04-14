#ifndef _KERNEL_HAL_X86_64_GDT_H
#define _KERNEL_HAL_X86_64_GDT_H 1

#include <stdint.h>

#define GDT_ENTRIES 8UL

#define CS_OFFSET          1UL
#define DATA_OFFSET        2UL
#define USER_CODE32_OFFSET 3UL
#define USER_DATA_OFFSET   4UL
#define USER_CODE_OFFSET   5UL
#define GDT_TSS_OFFSET     6UL

#define CS_SELECTOR          (CS_OFFSET * 8UL)
#define DATA_SELECTOR        (DATA_OFFSET * 8UL)
#define USER_CODE32_SELECTOR ((USER_CODE32_OFFSET * 8UL) | 0b11)
#define USER_DATA_SELECTOR   ((USER_DATA_OFFSET * 8UL) | 0b11)
#define USER_CODE_SELECTOR   ((USER_CODE_OFFSET * 8UL) | 0b11)
#define TSS_SELECTOR         (GDT_TSS_OFFSET * 8UL)

#define TSS_TYPE 0b10001001

#define TSS_RSP0_OFFSET 4
#define TSS_RSP1_OFFSET 12

#ifndef __ASSEMBLER__

#include <kernel/hal/x86/gdt.h>

struct processor;

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

void init_gdt(struct processor *processor);
void set_tss_stack_pointer(uintptr_t rsp);

#endif /* __ASSEMBLER__ */

#endif /* _KERNEL_HAL_X86_64_GDT_H */
