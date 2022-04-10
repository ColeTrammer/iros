#ifndef _KERNEL_HAL_I386_PROCESSOR_H
#define _KERNEL_HAL_I386_PROCESSOR_H 1

#include <kernel/hal/x86_64/gdt.h>

#ifdef __ASSEMBLER__
#define TSS_OFFSET 16
#else

#include <kernel/arch/x86_64/asm_utils.h>

struct processor;

struct arch_processor {
    struct tss tss;

    uint8_t acpi_id;
    uint8_t local_apic_id;

    struct gdt_entry gdt[GDT_ENTRIES];
    struct gdt_descriptor gdt_descriptor;
};

static inline struct processor *get_current_processor(void) {
    struct processor *processor;
    asm volatile("mov %%gs:0, %0" : "=r"(processor) : : "memory");
    return processor;
}
#endif /* __ASSEMBLER__ */

#endif /* _KERNEL_HAL_I386_PROCESSOR_H */
