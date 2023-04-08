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
    if (!global_state().processor_info.has_apic()) {
        println("Local APIC not supported, panicking..."_sv);
        ASSERT(false);
    }

    auto apic_msr = read_msr(ModelSpecificRegister::LocalApicBase);
    if (!(apic_msr & 0x800)) {
        println("Local APIC not enabled, panicking..."_sv);
        ASSERT(false);
    }

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

    // FIXME: actually enable the local APIC. This is currently disabled because the IO APIC is not yet implemented.
    // local_apic.write_spurious_interrupt_vector(255);
}
}
