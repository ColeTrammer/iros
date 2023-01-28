#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

namespace iris::mm {
static auto const heap_start = VirtualAddress((iris::mm::kernel_end.raw_address() / 4096 * 4096) + 4096);

Expected<VirtualAddress> AddressSpace::allocate_region(usize page_aligned_length) {
    // Basic hack algorithm: allocate the new region at a large fixed offset from the old region.
    // Additionally, immediately fill in the newly created pages.

    auto last_virtual_address = m_regions.back().transform(&Region::end).value_or(heap_start);
    auto new_virtual_address = last_virtual_address + 8192 * 0x1000;

    iris::println("sz={}"_sv, m_regions.size());

    // TODO: provide the flags as a parameter.
    auto [new_region, did_insert] = m_regions.emplace(
        new_virtual_address, page_aligned_length, RegionFlags::Readable | RegionFlags::Writable | RegionFlags::User);

    for (auto virtual_address : (*new_region).each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame())));
    }

    return (*new_region).base();
}

Expected<void> AddressSpace::allocate_region_at(VirtualAddress location, usize page_aligned_length) {
    // TODO: provide the flags as a parameter.
    auto [new_region, did_insert] = m_regions.emplace(
        location, page_aligned_length, RegionFlags::Readable | RegionFlags::Writable | RegionFlags::User);

    for (auto virtual_address : (*new_region).each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame())));
    }
    return {};
}
}