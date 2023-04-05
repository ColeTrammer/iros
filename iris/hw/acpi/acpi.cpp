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
    println("RSDP RSDT address: {}"_sv, rsdp.rsdt_address);

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

    for (auto entry : rsdt_entries) {
        auto& header = mm::map_physical_address(mm::PhysicalAddress(entry), sizeof(SDTHeader))->typed<SDTHeader>();
        auto signature = byte_array_to_string_view(header.signature);
        println("Found ACPI table: {}"_sv, signature);
    }
}
}
