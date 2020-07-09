#ifndef _KERNEL_HAL_X86_64_PROCESSOR_H
#define _KERNEL_HAL_X86_64_PROCESSOR_H 1

#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/gdt.h>

struct processor;

struct arch_processor {
    uint8_t acpi_id;
    uint8_t local_apic_id;

    struct gdt_entry gdt[GDT_ENTRIES];
    struct gdt_descriptor gdt_descriptor;
    struct tss tss;
};

static inline struct processor *get_current_processor(void) {
    struct processor *processor;
    asm volatile("mov %%gs:0, %0" : "=r"(processor) : : "memory");
    return processor;
}

#endif /* _KERNEL_HAL_X86_64_PROCESSOR_H */
