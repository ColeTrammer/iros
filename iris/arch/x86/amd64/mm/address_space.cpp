#include <di/container/view/prelude.h>
#include <di/math/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <iris/arch/x86/amd64/page_structure.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/error.h>
#include <iris/core/global_state.h>
#include <iris/core/preemption.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/physical_page.h>
#include <iris/mm/sections.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
using namespace x86::amd64;

static PageStructurePhysicalPage& init_as_page_structure_parent(PhysicalAddress address) {
    auto& page = physical_page(address);
    di::construct_at(&page.as_page_structure_page, PageStructurePhysicalPage::Parent {});
    return page.as_page_structure_page;
}

static PageStructurePhysicalPage& init_as_page_structure_leaf(PhysicalAddress address) {
    auto& page = physical_page(address);
    di::construct_at(&page.as_page_structure_page, PageStructurePhysicalPage::Leaf {});
    return page.as_page_structure_page;
};

static page_structure::VirtualAddressStructure decompose_virtual_address(VirtualAddress virtual_address) {
    return di::bit_cast<page_structure::VirtualAddressStructure>(virtual_address.raw_value());
}

AddressSpace::~AddressSpace() {
    auto& kernel_address_space = global_state().kernel_address_space;
    ASSERT(this != &kernel_address_space);

    // Load the kernel address space, to ensure we don't run in the current
    // address space as it is being destroyed.
    with_preemption_disabled([&] {
        kernel_address_space.load();

        // Unmap all regions in this address space.
        auto& locked = get_assuming_no_concurrent_accesses();
        auto end = locked.m_regions.end();
        for (auto it = locked.m_regions.begin(); it != end;) {
            auto& region = *it++;
            *locked.destroy_region(region.base(), region.length());
        }

        // Unmap the page table.
        deallocate_page_frame(architecture_page_table_base());
    });
}

Expected<void> LockedAddressSpace::destroy_region(VirtualAddress base, usize length) {
    auto it = this->m_regions.find(base);
    if (it == this->m_regions.end()) {
        println("WARNING: trying to unmap non-existent region"_sv);
        return di::Unexpected(Error::InvalidArgument);
    }
    auto object = it->backing_object().arc_from_this();
    m_regions.erase(it);

    for (auto page = base; page < base + length; page += 4096zu) {
        auto decomposed = decompose_virtual_address(page);
        auto pml4_offset = decomposed.get<page_structure::Pml4Offset>();
        auto& pml4 = TRY(map_physical_address(this->base().architecture_page_table_base(), 0x1000))
                         .typed<page_structure::PageStructureTable>();
        if (!pml4[pml4_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PML4)"_sv);
            continue;
        }

        auto pdp_page = PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12);
        auto pdp_offset = decomposed.get<page_structure::PdpOffset>();
        auto& pdp =
            TRY(map_physical_address(PhysicalAddress(pdp_page), 0x1000)).typed<page_structure::PageStructureTable>();
        if (!pdp[pdp_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PDP)"_sv);
            continue;
        }

        auto pd_page = PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12);
        auto pd_offset = decomposed.get<page_structure::PdOffset>();
        auto& pd = TRY(map_physical_address(PhysicalAddress(pd_page.raw_value()), 0x1000))
                       .typed<page_structure::PageStructureTable>();
        if (!pd[pd_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PD)"_sv);
            continue;
        }

        auto pt_page = PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12);
        auto pt_offset = decomposed.get<page_structure::PtOffset>();
        auto& pt = TRY(map_physical_address(PhysicalAddress(pt_page.raw_value()), 0x1000))
                       .typed<page_structure::PageStructureTable>();
        if (!pt[pt_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PT)"_sv);
            continue;
        }

        auto physical_address = PhysicalAddress(pt[pt_offset].get<page_structure::PhysicalAddress>() << 12);
        drop_page(physical_address);
        pt[pt_offset] = page_structure::StructureEntry(page_structure::Present(false));
        this->base().m_resident_pages.fetch_sub(1, di::MemoryOrder::Relaxed);

        // Make sure to cleanup the page structure tables if they are now empty.
        auto& pt_structure = page_structure_page(pt_page);
        if (--pt_structure.mapped_page_count == 0) {
            auto& pd_structure = page_structure_page(pd_page);

            pd[pd_offset] = page_structure::StructureEntry(page_structure::Present(false));
            pd_structure.children.erase(pt_structure);
            deallocate_page_frame(pt_page);

            if (pd_structure.children.empty()) {
                auto& pdp_structure = page_structure_page(pdp_page);

                pdp[pdp_offset] = page_structure::StructureEntry(page_structure::Present(false));
                pdp_structure.children.erase(pd_structure);
                deallocate_page_frame(pd_page);

                if (pdp_structure.children.empty()) {
                    auto& pml4_structure = page_structure_page(this->base().architecture_page_table_base());

                    pml4[pml4_offset] = page_structure::StructureEntry(page_structure::Present(false));
                    pml4_structure.children.erase(pdp_structure);
                    deallocate_page_frame(pdp_page);
                }
            }
        }
    }

    flush_tlb_global(base, length);
    return {};
}

void AddressSpace::load() {
    load_cr3(m_architecture_page_table_base.raw_value());
}

Expected<void> LockedAddressSpace::map_physical_page_early(VirtualAddress location, PhysicalAddress physical_address,
                                                           RegionFlags flags) {
    // NOTE: In the future, this function will not use the HHDM, which is only provided by Limine. To support other
    // bootloaders, we will need to use a different method to map pages early.
    auto const writable = !!(flags & RegionFlags::Writable);
    auto const not_executable = !(flags & RegionFlags::Executable);

    auto decomposed = decompose_virtual_address(location);
    auto pml4_offset = decomposed.get<page_structure::Pml4Offset>();
    auto& pml4 = TRY(map_physical_address(base().architecture_page_table_base(), 0x1000))
                     .typed<page_structure::PageStructureTable>();
    if (!pml4[pml4_offset].get<page_structure::Present>()) {
        pml4[pml4_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
    }

    auto pdp_offset = decomposed.get<page_structure::PdpOffset>();
    auto& pdp = TRY(map_physical_address(
                        PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                    .typed<page_structure::PageStructureTable>();
    if (!pdp[pdp_offset].get<page_structure::Present>()) {
        pdp[pdp_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
    }

    auto pd_offset = decomposed.get<page_structure::PdOffset>();
    auto& pd =
        TRY(map_physical_address(PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
            .typed<page_structure::PageStructureTable>();
    if (!pd[pd_offset].get<page_structure::Present>()) {
        pd[pd_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
    }

    auto pt_offset = decomposed.get<page_structure::PtOffset>();
    auto& pt =
        TRY(map_physical_address(PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
            .typed<page_structure::PageStructureTable>();
    if (pt[pt_offset].get<page_structure::Present>()) {
        println("WARNING: virtual address {} is already marked as present."_sv, location);
    }
    pt[pt_offset] = page_structure::StructureEntry(page_structure::PhysicalAddress(physical_address.raw_value() >> 12),
                                                   page_structure::Present(true), page_structure::Writable(writable),
                                                   page_structure::NotExecutable(not_executable));
    base().m_resident_pages.fetch_add(1, di::MemoryOrder::Relaxed);

    flush_tlb_global(location);

    return {};
}

Expected<void> LockedAddressSpace::map_physical_page(VirtualAddress location, PhysicalAddress physical_address,
                                                     RegionFlags flags) {
    // NOTE: The writable and not executable flags only apply at page granularity.
    //       Without any better knowledge, we have to assume some parts of the higher-level
    //       page will have mixed mappings, so try to set the page structure flags to be as
    //       permissive as possible.
    auto const writable = !!(flags & RegionFlags::Writable);
    auto const not_executable = !(flags & RegionFlags::Executable);
    auto const user = !!(flags & RegionFlags::User);

    auto decomposed = decompose_virtual_address(location);
    auto pml4_offset = decomposed.get<page_structure::Pml4Offset>();
    auto& pml4 = TRY(map_physical_address(base().architecture_page_table_base(), 0x1000))
                     .typed<page_structure::PageStructureTable>();
    auto& pml4_structure = page_structure_page(base().architecture_page_table_base());
    if (!pml4[pml4_offset].get<page_structure::Present>()) {
        pml4[pml4_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true), page_structure::User(user));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);

        auto pdp_page = PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12);
        auto& pdp_structure = init_as_page_structure_parent(pdp_page);
        pml4_structure.children.push_back(pdp_structure);
    }

    auto pdp_page = PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12);
    auto& pdp_structure = page_structure_page(pdp_page);

    auto pdp_offset = decomposed.get<page_structure::PdpOffset>();
    auto& pdp = TRY(map_physical_address(PhysicalAddress(pdp_page.raw_value()), 0x1000))
                    .typed<page_structure::PageStructureTable>();
    if (!pdp[pdp_offset].get<page_structure::Present>()) {
        pdp[pdp_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true), page_structure::User(user));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);

        auto pd_page = PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12);
        auto& pd_structure = init_as_page_structure_parent(pd_page);
        pdp_structure.children.push_back(pd_structure);
    }

    auto pd_page = PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12);
    auto& pd_structure = page_structure_page(pd_page);

    auto pd_offset = decomposed.get<page_structure::PdOffset>();
    auto& pd = TRY(map_physical_address(PhysicalAddress(pd_page.raw_value()), 0x1000))
                   .typed<page_structure::PageStructureTable>();
    if (!pd[pd_offset].get<page_structure::Present>()) {
        pd[pd_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true), page_structure::User(user));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);

        auto pt_page = PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12);
        auto& pt_structure = init_as_page_structure_leaf(pt_page);
        pd_structure.children.push_back(pt_structure);
    }

    auto pt_page = PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12);
    auto& pt_structure = page_structure_page(pt_page);

    auto pt_offset = decomposed.get<page_structure::PtOffset>();
    auto& pt = TRY(map_physical_address(PhysicalAddress(pt_page.raw_value()), 0x1000))
                   .typed<page_structure::PageStructureTable>();
    if (pt[pt_offset].get<page_structure::Present>()) {
        println("WARNING: virtual address {} is already marked as present."_sv, location);
    }
    pt[pt_offset] = page_structure::StructureEntry(
        page_structure::PhysicalAddress(physical_address.raw_value() >> 12), page_structure::Present(true),
        page_structure::Writable(writable), page_structure::User(user), page_structure::NotExecutable(not_executable));
    base().m_resident_pages.fetch_add(1, di::MemoryOrder::Relaxed);
    pt_structure.mapped_page_count++;

    bump_page(physical_address);

    flush_tlb_global(location);

    return {};
}

Expected<void> LockedAddressSpace::create_low_identity_mapping(VirtualAddress base, usize page_aligned_length) {
    for (auto address : di::iota(base, base + isize(page_aligned_length)) | di::stride(4096)) {
        auto physical_address = PhysicalAddress(address.raw_value());
        auto flags = RegionFlags::Readable | RegionFlags::Writable | RegionFlags::Executable;
        TRY(map_physical_page(address, physical_address, flags));
    }
    return {};
}

Expected<void> LockedAddressSpace::remove_low_identity_mapping(VirtualAddress base, usize page_aligned_length) {
    for (auto page = base; page < base + page_aligned_length; page += 4096zu) {
        auto decomposed = decompose_virtual_address(page);
        auto pml4_offset = decomposed.get<page_structure::Pml4Offset>();
        auto& pml4 = TRY(map_physical_address(this->base().architecture_page_table_base(), 0x1000))
                         .typed<page_structure::PageStructureTable>();
        if (!pml4[pml4_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PML4)"_sv);
            continue;
        }

        auto pdp_page = PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12);
        auto pdp_offset = decomposed.get<page_structure::PdpOffset>();
        auto& pdp =
            TRY(map_physical_address(PhysicalAddress(pdp_page), 0x1000)).typed<page_structure::PageStructureTable>();
        if (!pdp[pdp_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PDP)"_sv);
            continue;
        }

        auto pd_page = PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12);
        auto pd_offset = decomposed.get<page_structure::PdOffset>();
        auto& pd = TRY(map_physical_address(PhysicalAddress(pd_page.raw_value()), 0x1000))
                       .typed<page_structure::PageStructureTable>();
        if (!pd[pd_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PD)"_sv);
            continue;
        }

        auto pt_page = PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12);
        auto pt_offset = decomposed.get<page_structure::PtOffset>();
        auto& pt = TRY(map_physical_address(PhysicalAddress(pt_page.raw_value()), 0x1000))
                       .typed<page_structure::PageStructureTable>();
        if (!pt[pt_offset].get<page_structure::Present>()) {
            println("WARNING: trying to unmap non-present page (PT)"_sv);
            continue;
        }

        pt[pt_offset] = page_structure::StructureEntry(page_structure::Present(false));
        this->base().m_resident_pages.fetch_sub(1, di::MemoryOrder::Relaxed);

        // Make sure to cleanup the page structure tables if they are now empty.
        auto& pt_structure = page_structure_page(pt_page);
        if (--pt_structure.mapped_page_count == 0) {
            auto& pd_structure = page_structure_page(pd_page);

            pd[pd_offset] = page_structure::StructureEntry(page_structure::Present(false));
            pd_structure.children.erase(pt_structure);
            deallocate_page_frame(pt_page);

            if (pd_structure.children.empty()) {
                auto& pdp_structure = page_structure_page(pdp_page);

                pdp[pdp_offset] = page_structure::StructureEntry(page_structure::Present(false));
                pdp_structure.children.erase(pd_structure);
                deallocate_page_frame(pd_page);

                if (pdp_structure.children.empty()) {
                    auto& pml4_structure = page_structure_page(this->base().architecture_page_table_base());

                    pml4[pml4_offset] = page_structure::StructureEntry(page_structure::Present(false));
                    pml4_structure.children.erase(pdp_structure);
                    deallocate_page_frame(pdp_page);
                }
            }
        }
    }

    flush_tlb_global(base, page_aligned_length);
    return {};
}

Expected<void> LockedAddressSpace::setup_physical_memory_map(PhysicalAddress start, PhysicalAddress end,
                                                             VirtualAddress virtual_start) {
    auto decomposed = decompose_virtual_address(virtual_start);
    auto pml4_offset = decomposed.get<page_structure::Pml4Offset>();
    ASSERT_EQ(decomposed.get<page_structure::PdpOffset>(), 0);
    ASSERT_EQ(decomposed.get<page_structure::PdOffset>(), 0);
    ASSERT_EQ(decomposed.get<page_structure::PtOffset>(), 0);

    auto& pml4 = TRY(map_physical_address(base().architecture_page_table_base(), 0x1000))
                     .typed<page_structure::PageStructureTable>();
    if (!pml4[pml4_offset].get<page_structure::Present>()) {
        pml4[pml4_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
    }

    auto const& global_state = iris::global_state();
    auto phys_address = start;
    while (phys_address < end) {
        auto decomposed_phys_address = mm::decompose_virtual_address(VirtualAddress(phys_address.raw_value()));

        auto& pdp = TRY(map_physical_address(
                            PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                        .typed<page_structure::PageStructureTable>();
        auto pdp_offset = decomposed_phys_address.get<page_structure::PdpOffset>();

        if (global_state.processor_info.has_gib_pages()) {
            ASSERT(!pdp[pdp_offset].get<page_structure::Present>());
            pdp[pdp_offset] = page_structure::StructureEntry(
                page_structure::PhysicalAddress(phys_address.raw_value() >> 12), page_structure::Present(true),
                page_structure::Writable(true), page_structure::HugePage(true));
            base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
            phys_address += 1zu * 1024 * 1024 * 1024;
            continue;
        }

        if (!pdp[pdp_offset].get<page_structure::Present>()) {
            pdp[pdp_offset] = page_structure::StructureEntry(
                page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
                page_structure::Present(true), page_structure::Writable(true));
            base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
        }

        auto& pd = TRY(map_physical_address(
                           PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                       .typed<page_structure::PageStructureTable>();
        auto pd_offset = decomposed_phys_address.get<page_structure::PdOffset>();
        ASSERT(!pd[pd_offset].get<page_structure::Present>());
        pd[pd_offset] = page_structure::StructureEntry(page_structure::PhysicalAddress(phys_address.raw_value() >> 12),
                                                       page_structure::Present(true), page_structure::Writable(true),
                                                       page_structure::HugePage(true));
        base().m_structure_pages.fetch_add(1, di::MemoryOrder::Relaxed);
        phys_address += 2zu * 1024 * 1024;
    }

    return {};
}

Expected<void> LockedAddressSpace::setup_kernel_region(PhysicalAddress kernel_physical_start,
                                                       VirtualAddress kernel_virtual_start,
                                                       VirtualAddress kernel_virtual_end, RegionFlags flags) {
    for (auto offset = 0_u64; kernel_virtual_start + offset < kernel_virtual_end; offset += 4096) {
        auto physical_address = kernel_physical_start + offset;
        auto virtual_address = kernel_virtual_start + offset;
        TRY(map_physical_page_early(virtual_address, physical_address, flags));
    }
    return {};
}

Expected<void> LockedAddressSpace::bootstrap_kernel_page_tracking() {
    auto& global_state = global_state_in_boot();
    auto const max_physical_address = global_state.max_physical_address;
    auto const total_pages = di::divide_round_up(max_physical_address.raw_value(), 4096);
    auto const pages_needed_for_physical_pages = di::divide_round_up(total_pages * sizeof(PhysicalPage), 4096);

    auto& root = init_as_page_structure_parent(base().architecture_page_table_base());
    auto& pml4 = TRY(map_physical_address(base().architecture_page_table_base(), 0x1000))
                     .typed<page_structure::PageStructureTable>();

    // The initial kernel mappings have 6 regions.
    // 1. The physical memory direct map
    // 2. The physical page structures
    // 3. The kernel code
    // 4. The kernel read-only data
    // 5. The kernel read-write data
    // 6. The kernel heap (initially empty)

    auto backing_object_index = 0;
    auto region_index = 0;

    auto handle_physical_id_map = [&](VirtualAddress start, VirtualAddress end) -> Expected<void> {
        auto& region = global_state.inital_kernel_regions[region_index++];
        region.set_flags(RegionFlags::Readable | RegionFlags::Writable);
        region.set_base(start);
        region.set_length(end - start);
        m_regions.insert(region);

        auto decomposed_start = decompose_virtual_address(start);
        auto pml4_offset = decomposed_start.get<page_structure::Pml4Offset>();

        auto pdp_page = pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12;
        auto const pdp_entry_count = di::divide_round_up(end - start, 1024 * 1024 * 1024);
        TRY(map_physical_address(PhysicalAddress(pdp_page), 0x1000)).typed<page_structure::PageStructureTable>();
        if (global_state.processor_info.has_gib_pages()) {
            auto& pdp_structure = init_as_page_structure_leaf(PhysicalAddress(pdp_page));
            root.children.push_back(pdp_structure);

            pdp_structure.mapped_page_count = pdp_entry_count;
            return {};
        }

        auto& pdp =
            TRY(map_physical_address(PhysicalAddress(pdp_page), 0x1000)).typed<page_structure::PageStructureTable>();
        auto& pdp_structure = init_as_page_structure_parent(PhysicalAddress(pdp_page));
        root.children.push_back(pdp_structure);

        for (auto pdp_offset : di::range(pdp_entry_count)) {
            auto pd_page = pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12;

            auto& pd_structure = init_as_page_structure_leaf(PhysicalAddress(pd_page));
            pdp_structure.children.push_back(pd_structure);

            if (pdp_offset == pdp_entry_count - 1) {
                auto const last_pdp_entry_page_count =
                    di::divide_round_up((end - start) % (1024 * 1024 * 1024), 2 * 1024 * 1024);
                pd_structure.mapped_page_count = last_pdp_entry_page_count;
            } else {
                pd_structure.mapped_page_count = 512;
            }
        }
        return {};
    };

    auto handle_kernel_region = [&](VirtualAddress start, VirtualAddress end, RegionFlags flags) -> Expected<void> {
        auto& backing_object = global_state.inital_kernel_backing_objects[backing_object_index++];

        auto& region = global_state.inital_kernel_regions[region_index++];
        region.set_flags(flags);
        region.set_base(start);
        region.set_length(end - start);
        region.set_backing_object(backing_object.arc_from_this());
        m_regions.insert(region);

        auto start_decomposed = decompose_virtual_address(start);
        auto end_decomposed = decompose_virtual_address(end);

        auto start_position = start_decomposed.get<page_structure::Pml4Offset>() * 512_u64 * 512_u64 * 512_u64 +
                              start_decomposed.get<page_structure::PdpOffset>() * 512_u64 * 512_u64 +
                              start_decomposed.get<page_structure::PdOffset>() * 512_u64 +
                              start_decomposed.get<page_structure::PtOffset>();
        auto end_position = end_decomposed.get<page_structure::Pml4Offset>() * 512_u64 * 512_u64 * 512_u64 +
                            end_decomposed.get<page_structure::PdpOffset>() * 512_u64 * 512_u64 +
                            end_decomposed.get<page_structure::PdOffset>() * 512_u64 +
                            end_decomposed.get<page_structure::PtOffset>();

        auto indices =
            di::cartesian_product(di::range(512_u64), di::range(512_u64), di::range(512_u64), di::range(512_u64)) |
            di::drop(start_position) | di::take(end_position - start_position);
        for (auto [page_index, offsets] : di::enumerate(indices)) {
            auto [pml4_offset, pdp_offset, pd_offset, pt_offset] = offsets;
            auto pml4_entry = pml4[pml4_offset];
            ASSERT(pml4_entry.get<page_structure::Present>());
            auto pdp_page = PhysicalAddress(pml4_entry.get<page_structure::PhysicalAddress>() << 12);
            auto& pdp_structure = [&] -> PageStructurePhysicalPage& {
                auto it = di::find_if(root.children, [&](auto&& child) {
                    return physical_address(child) == pdp_page;
                });
                if (it != root.children.end()) {
                    return *it;
                }
                auto& result = init_as_page_structure_parent(pdp_page);
                root.children.push_back(result);
                return result;
            }();

            auto& pdp = TRY(map_physical_address(pdp_page, 0x1000)).typed<page_structure::PageStructureTable>();
            auto pdp_entry = pdp[pdp_offset];
            ASSERT(pdp_entry.get<page_structure::Present>());
            auto pd_page = PhysicalAddress(pdp_entry.get<page_structure::PhysicalAddress>() << 12);
            auto& pd_structure = [&] -> PageStructurePhysicalPage& {
                auto it = di::find_if(pdp_structure.children, [&](auto&& child) {
                    return physical_address(child) == pd_page;
                });
                if (it != pdp_structure.children.end()) {
                    return *it;
                }
                auto& result = init_as_page_structure_parent(pd_page);
                pdp_structure.children.push_back(result);
                return result;
            }();

            auto& pd = TRY(map_physical_address(pd_page, 0x1000)).typed<page_structure::PageStructureTable>();
            auto pd_entry = pd[pd_offset];
            ASSERT(pd_entry.get<page_structure::Present>());
            auto pt_page = PhysicalAddress(pd_entry.get<page_structure::PhysicalAddress>() << 12);
            auto& pt_structure = [&] -> PageStructurePhysicalPage& {
                auto it = di::find_if(pd_structure.children, [&](auto&& child) {
                    return physical_address(child) == pt_page;
                });
                if (it != pd_structure.children.end()) {
                    return *it;
                }
                auto& result = init_as_page_structure_leaf(pt_page);
                pd_structure.children.push_back(result);
                return result;
            }();

            pt_structure.mapped_page_count++;

            auto& pt = TRY(map_physical_address(pt_page, 0x1000)).typed<page_structure::PageStructureTable>();
            auto backing_page = PhysicalAddress(pt[pt_offset].get<page_structure::PhysicalAddress>() << 12);
            backing_object.get_assuming_no_concurrent_accesses().add_page(backing_page, page_index);
            bump_page(backing_page);
        }

        return {};
    };

    TRY(handle_physical_id_map(VirtualAddress(0xFFFF800000000000),
                               VirtualAddress(0xFFFF800000000000) + max_physical_address.raw_value()));

    TRY(handle_kernel_region(VirtualAddress(0xFFFF800000000000 + 4096_u64 * 512_u64 * 512_u64 * 512_u64),
                             VirtualAddress(0xFFFF800000000000 + 4096_u64 * 512_u64 * 512_u64 * 512_u64) +
                                 pages_needed_for_physical_pages * 4096,
                             RegionFlags::Readable | RegionFlags::Writable));

    TRY(handle_kernel_region(text_segment_start, text_segment_end, RegionFlags::Readable | RegionFlags::Executable));
    TRY(handle_kernel_region(rodata_segment_start, rodata_segment_end, RegionFlags::Readable));
    TRY(handle_kernel_region(data_segment_start, data_segment_end, RegionFlags::Readable | RegionFlags::Writable));

    TRY(handle_kernel_region(global_state.heap_start, global_state.heap_start,
                             RegionFlags::Readable | RegionFlags::Writable));

    return {};
}

void LockedAddressSpace::flush_tlb_global(VirtualAddress base, usize byte_length) {
    // SAFETY: We are protected by the address space lock.
    auto& current_processor = current_processor_unsafe();

    current_processor.flush_tlb_local(base, byte_length);

    if (global_state().all_aps_booted.load(di::MemoryOrder::Relaxed)) {
        current_processor.broadcast_ipi([&](IpiMessage& message) {
            message.tlb_flush_base = base;
            message.tlb_flush_size = byte_length;
        });
    }
}

Expected<di::Arc<AddressSpace>> create_empty_user_address_space() {
    // NOTE: allocate the address space first, so that the allocated page frame
    //       will not be leaked on failure.
    auto new_address_space = TRY(di::try_make_arc<AddressSpace>());

    auto new_pml4 = TRY(allocate_page_frame());
    new_address_space->set_architecture_page_table_base(new_pml4);
    init_as_page_structure_parent(new_pml4);

    auto& kernel_address_space = global_state().kernel_address_space;

    // The new address space should should consist of blank entries apart from those pml4 entries
    // which are shared with the kernel address space. We can thus simply copy the kernel address's
    // pml4 to the new address space.
    auto& destination = TRY(map_physical_address(new_pml4, 4096)).typed<page_structure::PageStructureTable>();
    auto& source = TRY(map_physical_address(kernel_address_space.architecture_page_table_base(), 4096))
                       .typed<page_structure::PageStructureTable>();

    di::copy(source, destination.data());

    return new_address_space;
}
}
