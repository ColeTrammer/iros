#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

namespace iris::mm {
Expected<VirtualAddress> AddressSpace::allocate_region(usize page_aligned_length, RegionFlags flags) {
    // Basic hack algorithm: allocate the new region at a large fixed offset from the old region.
    // Additionally, immediately fill in the newly created pages.

    if (m_kernel == !!(flags & RegionFlags::User)) {
        println("WARNING: attempt to allocate a region with mismatched userspace flag."_sv);
        return di::Unexpected(Error::InvalidArgument);
    }

    auto heap_start = global_state().heap_start;
    auto default_address = m_kernel ? heap_start : mm::VirtualAddress(0x10000000000);
    auto last_virtual_address = m_regions.back().transform(&Region::end).value_or(default_address);
    auto new_virtual_address = last_virtual_address + 8192 * 0x1000;

    auto [new_region, did_insert] = m_regions.emplace(new_virtual_address, page_aligned_length, flags);

    for (auto virtual_address : (*new_region).each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame()), flags));
    }

    return (*new_region).base();
}

Expected<void> AddressSpace::allocate_region_at(VirtualAddress location, usize page_aligned_length, RegionFlags flags) {
    auto [new_region, did_insert] = m_regions.emplace(location, page_aligned_length, flags);

    if (m_kernel == !!(flags & RegionFlags::User)) {
        println("WARNING: attempt to allocate a region with mismatched userspace flag."_sv);
        return di::Unexpected(Error::InvalidArgument);
    }

    for (auto virtual_address : (*new_region).each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame()), flags));
    }
    return {};
}

Expected<void> init_and_load_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                          VirtualAddress kernel_virtual_start,
                                                          PhysicalAddress max_physical_address) {
    auto& new_address_space = global_state_in_boot().kernel_address_space;
    new_address_space.set_architecture_page_table_base(TRY(allocate_page_frame()));
    new_address_space.set_kernel();

    for (auto physical_address = PhysicalAddress(0);
         physical_address < di::min(max_physical_address, PhysicalAddress(0x10000000ul)); physical_address += 0x1000) {
        TRY(new_address_space.map_physical_page(VirtualAddress(0xFFFF800000000000 + physical_address.raw_value()),
                                                physical_address,
                                                mm::RegionFlags::Readable | mm::RegionFlags::Writable));
    }

    for (auto virtual_address = text_segment_start; virtual_address < text_segment_end; virtual_address += 4096) {
        TRY(new_address_space.map_physical_page(
            virtual_address, PhysicalAddress(kernel_physical_start + (virtual_address - kernel_virtual_start)),
            mm::RegionFlags::Readable | mm::RegionFlags::Executable));
    }

    for (auto virtual_address = rodata_segment_start; virtual_address < rodata_segment_end; virtual_address += 4096) {
        TRY(new_address_space.map_physical_page(
            virtual_address, PhysicalAddress(kernel_physical_start + (virtual_address - kernel_virtual_start)),
            mm::RegionFlags::Readable));
    }

    for (auto virtual_address = data_segment_start; virtual_address < data_segment_end; virtual_address += 4096) {
        TRY(new_address_space.map_physical_page(
            virtual_address, PhysicalAddress(kernel_physical_start + (virtual_address - kernel_virtual_start)),
            mm::RegionFlags::Readable | mm::RegionFlags::Writable));
    }

    println("Loading new kernel address space..."_sv);
    new_address_space.load();
    return {};
}
}
