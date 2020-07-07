#include <string.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/gdt.h>
#include <kernel/hal/x86_64/idt.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>

void local_apic_send_eoi(void) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    local_apic->eoi_register = 0;
}

static void write_icr(volatile struct local_apic *local_apic, union local_apic_icr command) {
    local_apic->interrupt_command_register[1].value = (command.raw_value >> 32) & 0xFFFFFFFFU;
    local_apic->interrupt_command_register[0].value = command.raw_value & 0xFFFFFFFFU;
}

void init_ap(void) {
    debug_log("\n===============================\nPROCESSOR MADE IT TO THE KERNEL\n===============================\n");

    for (;;) {
    }
}

struct ap_trampoline {
    uint64_t cr3;
    uint64_t gdt;
    uint64_t idt;
    uint64_t rsp;
} __attribute__((packed));

static void start_ap(volatile struct local_apic *local_apic, struct processor *processor) {
    struct vm_region *code_trampoline = vm_allocate_low_identity_map(0x8000, KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START);
    assert(get_phys_addr(0x8000) == 0x8000);
    assert(code_trampoline->end > 0x8000 + KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START);

    struct vm_region *ap_stack = vm_allocate_kernel_region(PAGE_SIZE);
    memcpy((void *) code_trampoline->start, (void *) KERNEL_AP_TRAMPOLINE_START, KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START);

    struct ap_trampoline *trampoline =
        (void *) (code_trampoline->start + KERNEL_AP_TRAMPOLINE_END - KERNEL_AP_TRAMPOLINE_START - sizeof(struct ap_trampoline));
    trampoline->cr3 = get_cr3();
    trampoline->gdt = (uintptr_t) get_gdt_descriptor();
    trampoline->idt = (uintptr_t) get_idt_descriptor();
    trampoline->rsp = ap_stack->end;

    union local_apic_icr command = { .raw_value = 0 };
    command.destination_mode = LOCAL_APIC_ICR_DEST_MODE_INIT;
    command.init_level_de_assert = LOCAL_APIC_ICR_NO_DE_ASSERT;
    command.destination_type = LOCAL_APIC_ICR_DEST_TYPE_TARGETED;
    command.target_apic_id = processor->arch_processor.local_apic_id;

    debug_log("Sending INIT to CPU: [ %u, %#.16lX ]\n", processor->arch_processor.local_apic_id, trampoline->cr3);
    write_icr(local_apic, command);

    io_wait_us(10 * 1000);

    command.irq_number = 8;
    command.destination_mode = LOCAL_APIC_ICR_DEST_MODE_SIPI;
    for (int i = 0; i < 2; i++) {
        write_icr(local_apic, command);
        io_wait_us(200);
    }

    io_wait_us(25000000);

    vm_free_low_identity_map(code_trampoline);
    vm_free_kernel_region(ap_stack);
}

void local_apic_start_aps(void) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    struct processor *processor = get_processor_list();
    while (processor) {
        // NOTE: processor id 0 should be the BSP
        if (processor->id != 0) {
            start_ap(local_apic, processor);
        }

        processor = processor->next;
    }
}

void init_local_apic(void) {
    disable_pic();

    struct acpi_info *info = acpi_get_info();
    set_msr(MSR_LOCAL_APIC_BASE, info->local_apic_address | APIC_MSR_ENABLE_LOCAL);

    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);
    local_apic->spurious_interrupt_vector_register = 0x1FF;

    debug_log("Enabled local APIC\n");
}
