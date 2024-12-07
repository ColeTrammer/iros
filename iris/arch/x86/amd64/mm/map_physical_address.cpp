#include "iris/mm/map_physical_address.h"

#include "iris/core/global_state.h"
#include "iris/core/print.h"
#include "iris/mm/virtual_address.h"
#include "iris/third_party/limine.h"

extern "C" {
// HHDM refers to "higher-half direct map", which provides
// a complete mapping of all physical memory at a fixed virtual
// offset.
static limine_hhdm_request volatile hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = nullptr,
};
}

namespace iris::mm {
static bool yes = true;

auto map_physical_address(PhysicalAddress address, usize byte_size) -> Expected<PhysicalAddressMapping> {
    // FIXME: this only works with the bootloader's page tables for now.
    // FIXME: validate that the physical address mapping is reasonable (only up to 4 GiB if only 4 GiB of memory are
    // available).
    if (di::exchange(yes, false)) {
        iris::println("HHDM Base Address: {:#x}"_sv, hhdm_request.response->offset);
        global_state_in_boot().virtual_to_physical_offset = mm::VirtualAddress(hhdm_request.response->offset);
    }
    auto* virtual_address =
        reinterpret_cast<di::Byte*>(global_state().virtual_to_physical_offset.raw_value() + address.raw_value());
    return PhysicalAddressMapping({ virtual_address, byte_size });
}
}
