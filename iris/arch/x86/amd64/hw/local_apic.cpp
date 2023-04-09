#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/map_physical_address.h>

namespace iris::x86::amd64 {
LocalApic::LocalApic(mm::PhysicalAddress base) {
    m_base = &mm::map_physical_address(base, 0x1000)->typed<u32 volatile>();
}

void init_local_apic() {
    auto& global_state = global_state_in_boot();
    if (!global_state.arch_readonly_state.use_apic) {
        println("APIC support is disabled, so skipping local APIC initialization..."_sv);
        global_state.arch_readonly_state.use_apic = false;
        return;
    }

    if (!global_state.processor_info.has_apic()) {
        println("ACPI not detected, so skipping local APIC initialization..."_sv);
        global_state.arch_readonly_state.use_apic = false;
        return;
    }

    // Ensure the local APIC is enabled.
    auto apic_msr = read_msr(ModelSpecificRegister::LocalApicBase);
    apic_msr |= 0x800;
    write_msr(ModelSpecificRegister::LocalApicBase, apic_msr);

    auto apic_base = mm::PhysicalAddress(apic_msr & 0xFFFFF000);
    auto local_apic = LocalApic(apic_base);

    auto apic_id = local_apic.id();
    println("Local APIC ID: {}"_sv, apic_id);

    auto apic_version = local_apic.version();
    println("Local APIC version: {}"_sv, apic_version.get<ApicVersion>());
    println("Local APIC max LVT entry: {}"_sv, apic_version.get<ApicMaxLvtEntry>());
    println("Local APIC EOI extended register present: {}"_sv, apic_version.get<ApicExtendedRegisterPresent>());

    // Enable the local APIC by setting up the spurious interrupt vector register.
    // This also maps any spurious interrupts to IRQ 255.
    local_apic.write_spurious_interrupt_vector(0x1FF);

    global_state.boot_processor.arch_processor().set_local_apic(local_apic);
}
}
