#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/acpi.h>
#include <kernel/hal/x86_64/drivers/local_apic.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/mem/vm_allocator.h>

void local_apic_send_eoi(void) {
    struct acpi_info *info = acpi_get_info();
    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);

    local_apic->eoi_register = 0;
}

void init_local_apic(void) {
    disable_pic();

    struct acpi_info *info = acpi_get_info();
    set_msr(MSR_LOCAL_APIC_BASE, info->local_apic_address | APIC_MSR_ENABLE_LOCAL);

    volatile struct local_apic *local_apic = create_phys_addr_mapping(info->local_apic_address);
    local_apic->spurious_interrupt_vector_register = 0x1FF;

    debug_log("Enabled local APIC\n");
}
