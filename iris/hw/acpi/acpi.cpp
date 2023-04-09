#include <iris/arch/x86/amd64/hw/io_apic.h>
#include <iris/arch/x86/amd64/hw/local_apic.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/acpi/acpi.h>
#include <iris/hw/acpi/system_tables.h>
#include <iris/mm/map_physical_address.h>
#include <iris/third_party/limine.h>

namespace iris::acpi {
extern "C" {
static volatile limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
    .response = nullptr,
};
}

bool validate_acpi_checksum(di::Span<byte const> data) {
    auto checksum = di::fold_left(data | di::transform(di::to_underlying), 0, [](u8 a, u8 b) -> u8 {
        return a + b;
    });
    return checksum == 0;
}

void init_acpi() {
    println("Initializing ACPI..."_sv);

    if (!rsdp_request.response || !rsdp_request.response->address) {
        println("No RSDP found, panicking..."_sv);
        return;
    }

    // NOTE: Limine provides the RSDP address within the HHDM, which is preserved after the bootloader memory is
    // reclaimed. All pointers used for ACPI are physical addresses, so all other pointers will need to be converted to
    // point into the HHDM.
    auto& rsdp = *static_cast<RSDP*>(rsdp_request.response->address);

    auto signature = byte_array_to_string_view(rsdp.signature);
    auto oem_id = byte_array_to_string_view(rsdp.oem_id);

    println("RSDP signature: {}"_sv, signature);
    println("RSDP OEM ID: {}"_sv, oem_id);
    println("RSDP revision: {}"_sv, rsdp.revision);
    println("RSDP RSDT address: {}"_sv, auto(rsdp.rsdt_address));

    if (!rsdp.validate_v1()) {
        println("RSDP has invalid checksum, panicking..."_sv);
        ASSERT(false);
    }

    if (signature != "RSD PTR "_tsv) {
        println("RSDP has invalid signature, panicking..."_sv);
        ASSERT(false);
    }

    if (rsdp.revision >= 2) {
        println("RSDP revision is not yet supported, so the XSDT won't be used..."_sv);
    }

    auto& rsdt = mm::map_physical_address(mm::PhysicalAddress(rsdp.rsdt_address), sizeof(RSDT))->typed<RSDT>();
    auto rsdt_signature = byte_array_to_string_view(rsdt.signature);
    if (!rsdt.validate(sizeof(RSDT)) || rsdt_signature != "RSDT"_tsv) {
        println("RSDT is invalid, panicking..."_sv);
        ASSERT(false);
    }

    auto rsdt_entries = rsdt.entries();
    println("RSDT entries: {}"_sv, rsdt_entries.size());

    auto madt = di::Optional<MADT&> {};
    for (auto entry : rsdt_entries) {
        auto& header = mm::map_physical_address(mm::PhysicalAddress(entry), sizeof(SDTHeader))->typed<SDTHeader>();
        auto signature = byte_array_to_string_view(header.signature);
        println("Found ACPI table: {}"_sv, signature);

        if (signature == "APIC"_tsv) {
            if (!header.validate(sizeof(MADT))) {
                println("MADT header is invalid, panicking..."_sv);
                ASSERT(false);
            }

            madt = static_cast<MADT&>(header);
        }
    }

    if (!madt) {
        println("MADT not found, panicking..."_sv);
        ASSERT(false);
    }

    auto acpi_info = AcpiInformation {};
    acpi_info.madt = madt;

    for (auto interrupt_controller_structures = madt->interrupt_controller_structures();
         interrupt_controller_structures != interrupt_controller_structures.end(); ++interrupt_controller_structures) {
        auto const& interrupt_controller_structure = *interrupt_controller_structures;
        switch (interrupt_controller_structure.type) {
            case InterruptControllerStructureType::LocalApic: {
                if (!interrupt_controller_structures.validate(sizeof(ProcessorLocalApicStructure))) {
                    println("Local APIC structure is invalid, panicking..."_sv);
                    ASSERT(false);
                }

                auto const& local_apic =
                    static_cast<ProcessorLocalApicStructure const&>(interrupt_controller_structure);
                println("Found local APIC: {}/{}"_sv, local_apic.processor_id, local_apic.apic_id);

                *acpi_info.local_apic.push_back(local_apic);
                break;
            }
            case InterruptControllerStructureType::IoApic: {
                if (!interrupt_controller_structures.validate(sizeof(IoApicStructure))) {
                    println("I/O APIC structure is invalid, panicking..."_sv);
                    ASSERT(false);
                }

                auto const& io_apic = static_cast<IoApicStructure const&>(interrupt_controller_structure);
                println("Found I/O APIC: {} => IRQ {}"_sv, io_apic.io_apic_id, io_apic.global_system_interrupt_base);

                *acpi_info.io_apics.push_back(io_apic);
                break;
            }
            case InterruptControllerStructureType::InterruptSourceOverride: {
                if (!interrupt_controller_structures.validate(sizeof(InterruptSourceOverrideStructure))) {
                    println("Interrupt source override structure is invalid, panicking..."_sv);
                    ASSERT(false);
                }

                auto const& interrupt_source_override =
                    static_cast<InterruptSourceOverrideStructure const&>(interrupt_controller_structure);
                println("Found interrupt source override: Bus {}:{} => IRQ {} ({:#b})"_sv,
                        interrupt_source_override.bus, interrupt_source_override.source,
                        interrupt_source_override.global_system_interrupt, interrupt_source_override.flags.value);

                *acpi_info.interrupt_source_overrides.push_back(interrupt_source_override);
                break;
            }
            case InterruptControllerStructureType::LocalApicNmi: {
                if (!interrupt_controller_structures.validate(sizeof(LocalApicNmiStructure))) {
                    println("Local APIC NMI structure is invalid, panicking..."_sv);
                    ASSERT(false);
                }

                auto const& local_apic_nmi = static_cast<LocalApicNmiStructure const&>(interrupt_controller_structure);
                println("Found local APIC NMI: {} @ {} ({:#b})"_sv, local_apic_nmi.apic_processor_uid,
                        local_apic_nmi.local_apic_lint, local_apic_nmi.flags.value);

                acpi_info.local_apic_nmi = local_apic_nmi;
                break;
            }
            default: {
                if (!interrupt_controller_structure.validate(sizeof(InterruptControllerStructureHeader))) {
                    println("Unknown interrupt controller structure is invalid, panicking..."_sv);
                    ASSERT(false);
                }

                println("Found unknown system interrupt controller of type: {}"_sv,
                        di::to_underlying(interrupt_controller_structure.type));
                break;
            }
        }
    }

    global_state_in_boot().acpi_info = di::move(acpi_info);
}
}
