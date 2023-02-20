#include <iris/core/print.h>
#include <iris/mm/map_physical_address.h>
#include <limine.h>

extern "C" {
// HHDM refers to "higher-half direct map", which provides
// a complete mapping of all physical memory at a fixed virtual
// offset.
static volatile limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr,
};
}

namespace iris::mm {
static bool yes = true;
static uptr base = 0;

Expected<PhysicalAddressMapping> map_physical_address(PhysicalAddress address, usize byte_size) {
    // FIXME: this only works with the bootloader's page tables for now.
    // FIXME: validate that the physical address mapping is reasonable (only up to 4 GiB if only 4 GiB of memory are
    // available).
    if (di::exchange(yes, false)) {
        iris::println("HHDM Base Address: {:#x}"_sv, hhdm_request.response->offset);
        base = hhdm_request.response->offset;
    }
    auto virtual_address = reinterpret_cast<di::Byte*>(base + address.raw_address());
    return PhysicalAddressMapping({ virtual_address, byte_size });
}
}