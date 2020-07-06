#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/hal/x86_64/drivers/pic.h>
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

static void start_ap(volatile struct local_apic *local_apic, struct processor *processor) {
    union local_apic_icr command = { .raw_value = 0 };
    command.destination_mode = LOCAL_APIC_ICR_DEST_MODE_INIT;
    command.init_level_de_assert = LOCAL_APIC_ICR_DE_ASSERT;
    command.destination_type = LOCAL_APIC_ICR_DEST_TYPE_TARGETED;
    command.target_apic_id = processor->arch_processor.local_apic_id;

    debug_log("Sending INIT to CPU: [ %u ]\n", processor->arch_processor.local_apic_id);
    write_icr(local_apic, command);

    io_wait_us(10 * 1000);
}

void local_apic_start_aps(void) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    struct processor *processor = get_processor_list();
    while (processor) {
        // NOTE: processor id 0 should be the BSP
        if (processor->id != 0 && false) {
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
