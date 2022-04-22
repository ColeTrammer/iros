#ifndef _KERNEL_HAL_I386_PROCESSOR_H
#define _KERNEL_HAL_I386_PROCESSOR_H 1

#ifdef __ASSEMBLER__
#define TSS_OFFSET 16
#else

#include <kernel/arch/i686/asm_utils.h>
#include <kernel/hal/i686/gdt.h>

struct processor;

struct arch_processor {
    struct tss tss;

    uint8_t acpi_id;
    uint8_t local_apic_id;

    // Temp page related data
    uint32_t temp_page_irq_save;
    uint32_t temp_page_alloc_count;
    // NOTE: NULL for bsp, which uses static memory
    struct vm_region *temp_page_vm;
    struct vm_region *temp_page_page_table_vm;

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
