#include <stdint.h>
#include <string.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/i686/gdt.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/proc/task.h>

void init_gdt(struct processor *processor) {
    memset(processor->arch_processor.gdt, 0, GDT_ENTRIES * sizeof(struct gdt_entry));
    processor->arch_processor.gdt[CS_OFFSET].limit_low = 0xFFFF;
    processor->arch_processor.gdt[CS_OFFSET].limit_high = 0xF;
    processor->arch_processor.gdt[CS_OFFSET].type = 0b10011010;
    processor->arch_processor.gdt[CS_OFFSET].flags = 0b1100;

    processor->arch_processor.gdt[DATA_OFFSET].limit_low = 0xFFFF;
    processor->arch_processor.gdt[DATA_OFFSET].limit_high = 0xF;
    processor->arch_processor.gdt[DATA_OFFSET].type = 0b10010010;
    processor->arch_processor.gdt[DATA_OFFSET].flags = 0b1100;

    uintptr_t processor_vm_addr = (uintptr_t) processor;
    processor->arch_processor.gdt[GS_PROCESSOR_OFFSET].base_low = processor_vm_addr & 0xFFFF;
    processor->arch_processor.gdt[GS_PROCESSOR_OFFSET].base_mid = (processor_vm_addr >> 16) & 0xFF;
    processor->arch_processor.gdt[GS_PROCESSOR_OFFSET].base_high = (processor_vm_addr >> 24) & 0xFF;
    processor->arch_processor.gdt[GS_PROCESSOR_OFFSET].limit_low = 4;
    processor->arch_processor.gdt[GS_PROCESSOR_OFFSET].type = 0b10010010;
    processor->arch_processor.gdt[GS_PROCESSOR_OFFSET].flags = 0b0100;

    processor->arch_processor.gdt[USER_CODE_OFFSET].limit_low = 0xFFFF;
    processor->arch_processor.gdt[USER_CODE_OFFSET].limit_high = 0xF;
    processor->arch_processor.gdt[USER_CODE_OFFSET].type = 0b11111010;
    processor->arch_processor.gdt[USER_CODE_OFFSET].flags = 0b1100;

    processor->arch_processor.gdt[USER_DATA_OFFSET].limit_low = 0xFFFF;
    processor->arch_processor.gdt[USER_DATA_OFFSET].limit_high = 0xF;
    processor->arch_processor.gdt[USER_DATA_OFFSET].type = 0b11110010;
    processor->arch_processor.gdt[USER_DATA_OFFSET].flags = 0b1100;

    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].limit_low = 4;
    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].limit_high = 0xF;
    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].type = 0b11110010;
    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].flags = 0b0100;

    struct gdt_tss_entry *tss_entry = (struct gdt_tss_entry *) (processor->arch_processor.gdt + GDT_TSS_OFFSET);
    tss_entry->type = TSS_TYPE;
    tss_entry->limit_low = sizeof(struct tss);
    tss_entry->base_low = ((uintptr_t) &processor->arch_processor.tss) & 0x000000000000FFFF;
    tss_entry->base_low_mid = (((uintptr_t) &processor->arch_processor.tss) & 0x0000000000FF0000) >> 16;
    tss_entry->base_high_mid = (((uintptr_t) &processor->arch_processor.tss) & 0x00000000FF000000) >> 24;

    processor->arch_processor.gdt_descriptor.limit = GDT_ENTRIES * sizeof(struct gdt_entry) - 1;
    processor->arch_processor.gdt_descriptor.gdt = processor->arch_processor.gdt;
    load_gdt(processor->arch_processor.gdt_descriptor);

    asm volatile("ljmp %0, $_x\n"
                 "_x:\n"
                 "\tmov %1, %%dx\n"
                 "\tmov %%dx, %%ds\n"
                 "\tmov %%dx, %%es\n"
                 "\tmov %%dx, %%fs\n"
                 "\tmov %%dx, %%ss\n"
                 "\tmov %2, %%dx\n"
                 "\tmov %%dx, %%gs\n"
                 :
                 : "i"(CS_SELECTOR), "i"(DATA_SELECTOR), "i"(GS_PROCESSOR_SELECTOR)
                 : "memory", "edx");

    memset(&processor->arch_processor.tss, 0, sizeof(struct tss));
    processor->arch_processor.tss.io_map_base = sizeof(struct tss);
    load_tr(TSS_SELECTOR);
}

/* Must be called from unpremptable context */
void set_tss_stack_pointer(uintptr_t rsp) {
    get_current_processor()->arch_processor.tss.ss0 = DATA_SELECTOR;
    get_current_processor()->arch_processor.tss.esp0 = rsp;
}

/* Must be called from unpremptable context */
void arch_task_load_thread_self_pointer(void *thread_self_pointer) {
    struct processor *processor = get_current_processor();
    uintptr_t vm_addr = (uintptr_t) thread_self_pointer;

    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].base_low = vm_addr & 0xFFFF;
    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].base_mid = (vm_addr >> 16) & 0xFF;
    processor->arch_processor.gdt[GS_USER_THREAD_OFFSET].base_high = (vm_addr >> 24) & 0xFF;
}
