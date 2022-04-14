#ifndef _KERNEL_HAL_I686_GDT_H
#define _KERNEL_HAL_I686_GDT_H 1

#include <stdint.h>

#define GDT_ENTRIES 8UL

#define CS_OFFSET        1UL
#define DATA_OFFSET      2UL
#define USER_CODE_OFFSET 3UL
#define USER_DATA_OFFSET 4UL
#define GDT_TSS_OFFSET   5UL

#define CS_SELECTOR        (CS_OFFSET * 8UL)
#define DATA_SELECTOR      (DATA_OFFSET * 8UL)
#define USER_CODE_SELECTOR ((USER_CODE_OFFSET * 8UL) | 0b11)
#define USER_DATA_SELECTOR ((USER_DATA_OFFSET * 8UL) | 0b11)
#define TSS_SELECTOR       (GDT_TSS_OFFSET * 8UL)

#define TSS_TYPE 0b10001001

#ifndef __ASSEMBLER__

#include <kernel/hal/x86/gdt.h>

struct processor;

struct tss {
    uint16_t link;
    uint16_t reserved0;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t reserved1;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t reserved2;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t reserved3;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t reserved4;
    uint16_t cs;
    uint16_t reserved5;
    uint16_t ss;
    uint16_t reserved6;
    uint16_t ds;
    uint16_t reserved7;
    uint16_t fs;
    uint16_t reserved8;
    uint16_t gs;
    uint16_t reserved9;
    uint16_t ldtr;
    uint16_t reserved10;
    uint16_t reserved11;
    uint16_t io_map_base;
    uint32_t ssp;
} __attribute__((packed));

void init_gdt(struct processor *processor);
void set_tss_stack_pointer(uintptr_t rsp);

#endif /* __ASSEMBLER__ */

#endif /* _KERNEL_HAL_I686_GDT_H */
