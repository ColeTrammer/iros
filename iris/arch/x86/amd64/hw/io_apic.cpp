#include <iris/arch/x86/amd64/hw/io_apic.h>
#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/arch/x86/amd64/msr.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/map_physical_address.h>

namespace iris::x86::amd64 {
static_assert(di::Impl<IoApic, IrqControllerInterface>);

IoApic::IoApic(mm::PhysicalAddress access_base, u8 global_offset) : m_global_offset(global_offset) {
    m_access = &mm::map_physical_address(access_base, 0x1000)->typed<u32 volatile>();

    m_id = (direct_read(IoApicOffset::Id) & 0x0F000000) >> 24;

    auto version = di::bit_cast<IoApicVersionRegister>(direct_read(IoApicOffset::Version));
    m_max_redirection_entry = version.get<IoApicMaxRedirectionEntry>();

    println("IO APIC ID: {}"_sv, m_id);
    println("IO APIC version: {}"_sv, version.get<IoApicVersion>());
    println("IO APIC max redirection entry: {}"_sv, m_max_redirection_entry);

    for (auto i : di::range(m_max_redirection_entry + 1u)) {
        write_redirection_entry(i, IoApicRedirectionTableEntry(IoApicRedirectionTableEntryMask(true)));
    }
}

u32 IoApic::direct_read(IoApicOffset offset) {
    m_access[di::to_underlying(IoApicAccessOffset::RegisterSelect) / sizeof(u32)] = di::to_underlying(offset);
    return m_access[di::to_underlying(IoApicAccessOffset::Window) / sizeof(u32)];
}

void IoApic::direct_write(IoApicOffset offset, u32 value) {
    m_access[di::to_underlying(IoApicAccessOffset::RegisterSelect) / sizeof(u32)] = di::to_underlying(offset);
    m_access[di::to_underlying(IoApicAccessOffset::Window) / sizeof(u32)] = value;
}

void tag_invoke(di::Tag<send_eoi>, IoApic&, IrqLine) {
    // SAFETY: This is safe because send_eoi must be called in interrupt context.
    current_processor_unsafe().arch_processor().local_apic().send_eoi();
}

void tag_invoke(di::Tag<disable_irq_line>, IoApic& self, IrqLine irq_line) {
    auto [relative_irq_line, _] = self.resolve_irq_line(irq_line);
    auto redirection_entry = self.read_redirection_entry(relative_irq_line);
    redirection_entry.set<IoApicRedirectionTableEntryMask>(true);
    self.write_redirection_entry(relative_irq_line, redirection_entry);
}

void tag_invoke(di::Tag<enable_irq_line>, IoApic& self, IrqLine irq_line) {
    auto [relative_irq_line, interrupt_source_override] = self.resolve_irq_line(irq_line);

    auto [polarity, trigger_mode] = [&]() -> di::Tuple<bool, bool> {
        auto polarity = interrupt_source_override
                            .transform(di::compose(&acpi::MPSInterruptFlags::polarity,
                                                   &acpi::InterruptSourceOverrideStructure::flags))
                            .value_or(acpi::MPSInterruptFlags::Polarity::BusDefined);

        auto trigger_mode = interrupt_source_override
                                .transform(di::compose(&acpi::MPSInterruptFlags::trigger_mode,
                                                       &acpi::InterruptSourceOverrideStructure::flags))
                                .value_or(acpi::MPSInterruptFlags::TriggerMode::BusDefined);

        // By default, assume that the interrupt is active high and edge-triggered.
        if (polarity == acpi::MPSInterruptFlags::Polarity::BusDefined ||
            polarity == acpi::MPSInterruptFlags::Polarity::Reserved) {
            polarity = acpi::MPSInterruptFlags::Polarity::ActiveHigh;
        }
        if (trigger_mode == acpi::MPSInterruptFlags::TriggerMode::BusDefined ||
            trigger_mode == acpi::MPSInterruptFlags::TriggerMode::Reserved) {
            trigger_mode = acpi::MPSInterruptFlags::TriggerMode::Edge;
        }

        // NOTE: Active high and edge triggered both correspond to a bit value of 0.
        return { polarity != acpi::MPSInterruptFlags::Polarity::ActiveHigh,
                 trigger_mode != acpi::MPSInterruptFlags::TriggerMode::Edge };
    }();

    auto redirection_entry = IoApicRedirectionTableEntry {};

    // Set the interrupt vector.
    redirection_entry.set<IoApicRedirectionTableEntryVector>(
        (global_state().arch_readonly_state.external_irq_offset + i8(irq_line.raw_value())).raw_value());

    // Set the delivery mode to fixed.
    redirection_entry.set<IoApicRedirectionTableEntryDeliveryMode>(ApicMessageType::Fixed);

    // Set the polarity and trigger mode.
    redirection_entry.set<IoApicRedirectionTableEntryPolarity>(polarity);
    redirection_entry.set<IoApicRedirectionTableEntryTriggerMode>(trigger_mode);

    // For now, send the interrupt to the BSP.
    redirection_entry.set<IoApicRedirectionTableEntryDestination>(0);
    redirection_entry.set<IoApicRedirectionTableEntryDestinationMode>(IoApicDestinationMode::Physical);

    // Enable the interrupt.
    redirection_entry.set<IoApicRedirectionTableEntryMask>(false);

    self.write_redirection_entry(relative_irq_line, redirection_entry);
}

IrqLineRange tag_invoke(di::Tag<responsible_irq_line_range>, IoApic const& self) {
    return IrqLineRange(IrqLine(self.m_global_offset), IrqLine(self.m_global_offset + self.m_max_redirection_entry));
}

di::Tuple<u8, di::Optional<acpi::InterruptSourceOverrideStructure>> IoApic::resolve_irq_line(IrqLine irq_line) {
    for (auto const& interrupt_source_override : global_state().acpi_info->interrupt_source_overrides) {
        if (interrupt_source_override.source == irq_line.raw_value()) {
            // FIXME: this code assumes that the interrupt source override doesn't cause the IRQ line to be remapped to
            // an entirely different IO APIC.
            ASSERT_GT_EQ(interrupt_source_override.global_system_interrupt, m_global_offset);
            ASSERT_LT_EQ(interrupt_source_override.global_system_interrupt, m_global_offset + m_max_redirection_entry);

            return { interrupt_source_override.global_system_interrupt - m_global_offset, interrupt_source_override };
        }
    }
    return { irq_line.raw_value() - m_global_offset, di::nullopt };
}

void init_io_apic() {
    auto& global_state = iris::global_state_in_boot();
    if (!global_state.acpi_info || global_state.acpi_info->io_apics.empty()) {
        println("ACPI not available, skipping IO APIC initialization"_sv);
        global_state.arch_readonly_state.use_apic = false;
        return;
    }

    if (!global_state.arch_readonly_state.use_apic) {
        println("APIC not enabled, skipping IO APIC initialization"_sv);
        global_state.arch_readonly_state.use_apic = false;
        return;
    }

    auto const& acpi_info = *global_state.acpi_info;
    for (auto const& io_apic : acpi_info.io_apics) {
        *global_state.irq_controllers.emplace_back(
            IoApic(mm::PhysicalAddress(io_apic.io_apic_address), io_apic.global_system_interrupt_base));
    }

    // Don't use 32, as that's reserved by the PIC.
    global_state.arch_readonly_state.external_irq_offset = GlobalIrqNumber(64);
}
}
