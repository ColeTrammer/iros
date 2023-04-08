#include <iris/arch/x86/amd64/hw/io_apic.h>
#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/map_physical_address.h>

namespace iris::x86::amd64 {
IoApic::IoApic(mm::PhysicalAddress access_base, u8 global_offset) : m_global_offset(global_offset) {
    m_access = &mm::map_physical_address(access_base, 0x1000)->typed<u32 volatile>();

    m_id = (direct_read(IoApicOffset::Id) & 0x0F000000) >> 24;

    auto version = di::bit_cast<IoApicVersionRegister>(direct_read(IoApicOffset::Version));
    m_max_redirection_entry = version.get<IoApicMaxRedirectionEntry>();

    println("IO APIC ID: {}"_sv, m_id);
    println("IO APIC version: {}"_sv, version.get<IoApicVersion>());
    println("IO APIC max redirection entry: {}"_sv, m_max_redirection_entry);
}

u32 IoApic::direct_read(IoApicOffset offset) {
    m_access[di::to_underlying(IoApicAccessOffset::RegisterSelect) / sizeof(u32)] = di::to_underlying(offset);
    return m_access[di::to_underlying(IoApicAccessOffset::Window) / sizeof(u32)];
}

void IoApic::direct_write(IoApicOffset offset, u32 value) {
    m_access[di::to_underlying(IoApicAccessOffset::RegisterSelect) / sizeof(u32)] = di::to_underlying(offset);
    m_access[di::to_underlying(IoApicAccessOffset::Window) / sizeof(u32)] = value;
}

void init_io_apic() {
    auto const& global_state = iris::global_state();
    if (!global_state.acpi_info) {
        println("ACPI not available, skipping IO APIC initialization"_sv);
        return;
    }

    auto const& acpi_info = *global_state.acpi_info;
    for (auto const& io_apic : acpi_info.io_apics) {
        (void) IoApic(mm::PhysicalAddress(io_apic.io_apic_address), io_apic.global_system_interrupt_base);
    }
}
}
