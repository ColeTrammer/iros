#include <iris/arch/x86/amd64/page_structure.h>
#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>

namespace iris::mm {
using namespace x86::amd64;

page_structure::VirtualAddressStructure decompose_virtual_address(VirtualAddress virtual_address) {
    return di::bit_cast<page_structure::VirtualAddressStructure>(virtual_address.raw_value());
}

AddressSpace::~AddressSpace() {
    auto& kernel_address_space = global_state().kernel_address_space;
    ASSERT(this != &kernel_address_space);

    // Load the kernel address space, to ensure we don't run in the current
    // address space as it is being destroyed.
    asm volatile("cli");
    kernel_address_space.load();

    auto from_physical_address = [](PhysicalAddress physical_address) {
        // NOTE: as long as this page table is not corrupted, mapping the physical page will always succeed.
        //       It is therefore OK to assert there are no issues creating the mapping.
        auto result = map_physical_address(physical_address, 4096);
        ASSERT(result);
        return &result->typed<page_structure::PageStructureTable const>();
    };

    auto free_all_user_pages = di::ycombinator([&](auto&& self, page_structure::StructureEntry physical_page_structure,
                                                   int depth = 0) -> void {
        // Perform a post-order traversal over every physical page allocated in this layer. At depth 4, the physical
        // pages no longer refer to other pages, and so we stop the recursion explicitly. The iteration only
        // considers present pages which are not owned by the kernel.
        auto physical_address = PhysicalAddress(physical_page_structure.get<page_structure::PhysicalAddress>() << 12);
        if (depth < 4) {
            auto* structure = from_physical_address(physical_address);
            for (auto& entry : *structure) {
                if (!entry.get<page_structure::Present>() || !entry.get<page_structure::User>()) {
                    continue;
                }
                self(entry, depth + 1);
            }
        }

        deallocate_page_frame(physical_address);
    });

    free_all_user_pages(di::bit_cast<page_structure::StructureEntry>(architecture_page_table_base()));
}

void AddressSpace::load() {
    load_cr3(m_architecture_page_table_base.raw_value());
}

Expected<void> AddressSpace::map_physical_page(VirtualAddress location, PhysicalAddress physical_address,
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
    auto& pml4 =
        TRY(map_physical_address(architecture_page_table_base(), 0x1000)).typed<page_structure::PageStructureTable>();
    if (!pml4[pml4_offset].get<page_structure::Present>()) {
        pml4[pml4_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true), page_structure::User(user));
        ++m_structure_pages;
    }

    auto pdp_offset = decomposed.get<page_structure::PdpOffset>();
    auto& pdp = TRY(map_physical_address(
                        PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                    .typed<page_structure::PageStructureTable>();
    if (!pdp[pdp_offset].get<page_structure::Present>()) {
        pdp[pdp_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true), page_structure::User(user));
        ++m_structure_pages;
    }

    auto pd_offset = decomposed.get<page_structure::PdOffset>();
    auto& pd =
        TRY(map_physical_address(PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
            .typed<page_structure::PageStructureTable>();
    if (!pd[pd_offset].get<page_structure::Present>()) {
        pd[pd_offset] = page_structure::StructureEntry(
            page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_value() >> 12),
            page_structure::Present(true), page_structure::Writable(true), page_structure::User(user));
        ++m_structure_pages;
    }

    auto pt_offset = decomposed.get<page_structure::PtOffset>();
    auto& pt =
        TRY(map_physical_address(PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
            .typed<page_structure::FinalTable>();
    if (pt[pt_offset].get<page_structure::Present>()) {
        println("WARNING: virtual address {} is already marked as present."_sv, location);
    }
    pt[pt_offset] = page_structure::StructureEntry(
        page_structure::PhysicalAddress(physical_address.raw_value() >> 12), page_structure::Present(true),
        page_structure::Writable(writable), page_structure::User(user), page_structure::NotExecutable(not_executable));
    ++m_resident_pages;

    return {};
}

Expected<di::Arc<AddressSpace>> create_empty_user_address_space() {
    // NOTE: allocate the address space first, so that the allocated page frame
    //       will not be leaked on failure.
    auto new_address_space = TRY(di::try_make_arc<AddressSpace>());

    auto new_pml4 = TRY(allocate_page_frame());
    new_address_space->set_architecture_page_table_base(new_pml4);

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
