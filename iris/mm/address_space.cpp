#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

namespace iris::mm {
static auto const heap_start = VirtualAddress((kernel_end.raw_address() / 4096 * 4096) + 4096);

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

Expected<AddressSpace> create_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                           VirtualAddress kernel_virtual_start,
                                                           PhysicalAddress max_physical_address) {
    auto new_address_space = AddressSpace(allocate_page_frame()->raw_address());

    for (auto physical_address = PhysicalAddress(0);
         physical_address < di::min(max_physical_address, PhysicalAddress(0x1000000ul)); physical_address += 0x1000) {
        TRY(new_address_space.map_physical_page(VirtualAddress(0xFFFF800000000000 + physical_address.raw_address()),
                                                physical_address));
    }

    for (auto virtual_address = text_segment_start; virtual_address < text_segment_end; virtual_address += 4096) {
        TRY(new_address_space.map_physical_page(
            virtual_address, PhysicalAddress(kernel_physical_start + (virtual_address - kernel_virtual_start))));
    }

    for (auto virtual_address = rodata_segment_start; virtual_address < rodata_segment_end; virtual_address += 4096) {
        TRY(new_address_space.map_physical_page(
            virtual_address, PhysicalAddress(kernel_physical_start + (virtual_address - kernel_virtual_start))));
    }

    for (auto virtual_address = data_segment_start; virtual_address < data_segment_end; virtual_address += 4096) {
        TRY(new_address_space.map_physical_page(
            virtual_address, PhysicalAddress(kernel_physical_start + (virtual_address - kernel_virtual_start))));
    }

    return new_address_space;
}
}