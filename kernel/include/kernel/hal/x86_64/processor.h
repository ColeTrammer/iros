#ifndef _KERNEL_HAL_X86_64_PROCESSOR_H
#define _KERNEL_HAL_X86_64_PROCESSOR_H 1

#include <stdint.h>

#include <kernel/hal/x86_64/gdt.h>

struct arch_processor {
    uint8_t acpi_id;
    uint8_t local_apic_id;

    struct gdt_entry gdt[GDT_ENTRIES];
    struct gdt_descriptor gdt_descriptor;
    struct tss tss;
};

#endif /* _KERNEL_HAL_X86_64_PROCESSOR_H */
