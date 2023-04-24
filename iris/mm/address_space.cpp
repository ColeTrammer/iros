#include <di/math/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/physical_page.h>
#include <iris/mm/sections.h>

namespace iris::mm {
AddressSpace& LockedAddressSpace::base() {
    return static_cast<AddressSpace&>(
        reinterpret_cast<di::Synchronized<LockedAddressSpace, InterruptibleSpinlock>&>(*this));
}

Expected<VirtualAddress> LockedAddressSpace::allocate_region(di::Box<Region> region) {
    // Basic hack algorithm: allocate the new region at a large fixed offset from the old region.
    // Additionally, immediately fill in the newly created pages.

    auto flags = region->flags();
    auto page_aligned_length = region->length();

    if (base().m_kernel == !!(flags & RegionFlags::User)) {
        println("WARNING: attempt to allocate a region with mismatched userspace flag."_sv);
        return di::Unexpected(Error::InvalidArgument);
    }

    if (page_aligned_length >= 4_u64 * 1024 * 1024 * 1024) {
        println("WARNING: attempt to allocate extremely large region of size {:x}."_sv, page_aligned_length);
        return di::Unexpected(Error::NotEnoughMemory);
    }

    auto heap_start = global_state().heap_start;
    auto default_address = base().m_kernel ? heap_start : mm::VirtualAddress(0x10000000000);
    auto last_virtual_address = m_regions.back().transform(&Region::end).value_or(default_address);
    auto new_virtual_address = last_virtual_address + 8192zu * 0x1000zu;

    region->set_base(new_virtual_address);
    auto [new_region, did_insert] = m_regions.insert(*region.release());

    for (auto virtual_address : new_region->each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame()), flags));
    }

    return new_region->base();
}

Expected<void> LockedAddressSpace::allocate_region_at(di::Box<Region> region) {
    auto flags = region->flags();

    auto [new_region, did_insert] = m_regions.insert(*region.release());

    if (base().m_kernel == !!(flags & RegionFlags::User)) {
        println("WARNING: attempt to allocate a region with mismatched userspace flag."_sv);
        return di::Unexpected(Error::InvalidArgument);
    }

    for (auto virtual_address : new_region->each_page()) {
        TRY(map_physical_page(virtual_address, TRY(allocate_page_frame()), flags));
    }
    return {};
}

Expected<VirtualAddress> AddressSpace::allocate_region(usize page_aligned_length, RegionFlags flags) {
    auto region = TRY(di::try_box<Region>(VirtualAddress(0), page_aligned_length, flags));
    return lock()->allocate_region(di::move(region));
}

Expected<void> AddressSpace::allocate_region_at(VirtualAddress location, usize page_aligned_length, RegionFlags flags) {
    auto region = TRY(di::try_box<Region>(location, page_aligned_length, flags));
    return lock()->allocate_region_at(di::move(region));
}

Expected<void> init_and_load_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                          VirtualAddress kernel_virtual_start,
                                                          PhysicalAddress max_physical_address) {
    auto& global_state = global_state_in_boot();
    auto const total_pages = di::divide_round_up(max_physical_address.raw_value(), 4096);
    auto pages_needed_for_physical_pages = di::divide_round_up(total_pages * sizeof(PhysicalPage), 4096);
    auto allocated_phys_base = global_state.allocated_physical_page_base =
        TRY(allocate_physically_contiguous_page_frames(pages_needed_for_physical_pages));
    println("Preparing to manage {} total pages."_sv, total_pages);
    println("Allocated {} physical pages at {} for physical page structures."_sv, pages_needed_for_physical_pages,
            allocated_phys_base);

    auto& new_address_space = global_state.kernel_address_space;
    new_address_space.set_architecture_page_table_base(TRY(allocate_page_frame()));
    new_address_space.set_kernel();

    TRY(new_address_space.get_assuming_no_concurrent_accesses().setup_physical_memory_map(
        PhysicalAddress(0), max_physical_address, VirtualAddress(0xFFFF800000000000)));

    println("Mapping physical pages at {}."_sv,
            VirtualAddress(0xFFFF800000000000_u64 + 4096_u64 * 512_u64 * 512_u64 * 512_u64));
    TRY(new_address_space.get_assuming_no_concurrent_accesses().setup_kernel_region(
        allocated_phys_base, VirtualAddress(0xFFFF800000000000_u64 + 4096_u64 * 512_u64 * 512_u64 * 512_u64),
        VirtualAddress(0xFFFF800000000000_u64 + 4096_u64 * 512_u64 * 512_u64 * 512_u64) +
            pages_needed_for_physical_pages * 4096,
        RegionFlags::Readable | RegionFlags::Writable));

    TRY(new_address_space.get_assuming_no_concurrent_accesses().setup_kernel_region(
        PhysicalAddress(kernel_physical_start + (text_segment_start - kernel_virtual_start)), text_segment_start,
        text_segment_end, RegionFlags::Readable | RegionFlags::Executable));

    TRY(new_address_space.get_assuming_no_concurrent_accesses().setup_kernel_region(
        PhysicalAddress(kernel_physical_start + (rodata_segment_start - kernel_virtual_start)), rodata_segment_start,
        rodata_segment_end, RegionFlags::Readable));

    TRY(new_address_space.get_assuming_no_concurrent_accesses().setup_kernel_region(
        PhysicalAddress(kernel_physical_start + (data_segment_start - kernel_virtual_start)), data_segment_start,
        data_segment_end, RegionFlags::Readable | RegionFlags::Writable));

    println("Loading new kernel address space..."_sv);
    new_address_space.load();

    TRY(new_address_space.get_assuming_no_concurrent_accesses().bootstrap_kernel_page_tracking());
    return {};
}
}
