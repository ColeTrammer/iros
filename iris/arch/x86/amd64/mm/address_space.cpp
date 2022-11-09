#include <iris/arch/x86/amd64/page_structure.h>
#include <iris/core/log.h>
#include <iris/mm/address_space.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>

namespace iris::mm {
using namespace x86::amd64;

page_structure::VirtualAddressStructure decompose_virtual_address(VirtualAddress virtual_address) {
    return di::bit_cast<page_structure::VirtualAddressStructure>(virtual_address.raw_address());
}

Expected<void> AddressSpace::map_physical_page(VirtualAddress location, PhysicalAddress physical_address) {
    auto pml4_or_error = map_physical_address(PhysicalAddress(architecture_page_table_base()), 0x1000);
    if (!pml4_or_error) {
        return di::Unexpected(pml4_or_error.error());
    }

    auto decomposed = decompose_virtual_address(location);
    auto pml4_offset = decomposed.get<page_structure::Pml4Offset>();
    auto& pml4 =
        TRY(map_physical_address(PhysicalAddress(architecture_page_table_base()), 0x1000)).typed<page_structure::PageStructureTable>();
    if (!pml4[pml4_offset].get<page_structure::Present>()) {
        pml4[pml4_offset] = page_structure::StructureEntry(page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_address() >> 12),
                                                           page_structure::Present(true), page_structure::Writable(true));
    }

    auto pdp_offset = decomposed.get<page_structure::PdpOffset>();
    auto& pdp = TRY(map_physical_address(PhysicalAddress(pml4[pml4_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                    .typed<page_structure::PageStructureTable>();
    if (!pdp[pdp_offset].get<page_structure::Present>()) {
        pdp[pdp_offset] = page_structure::StructureEntry(page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_address() >> 12),
                                                         page_structure::Present(true), page_structure::Writable(true));
    }

    auto pd_offset = decomposed.get<page_structure::PdOffset>();
    auto& pd = TRY(map_physical_address(PhysicalAddress(pdp[pdp_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                   .typed<page_structure::PageStructureTable>();
    if (!pd[pd_offset].get<page_structure::Present>()) {
        pd[pd_offset] = page_structure::StructureEntry(page_structure::PhysicalAddress(TRY(allocate_page_frame()).raw_address() >> 12),
                                                       page_structure::Present(true), page_structure::Writable(true));
    }

    auto pt_offset = decomposed.get<page_structure::PtOffset>();
    auto& pt = TRY(map_physical_address(PhysicalAddress(pd[pd_offset].get<page_structure::PhysicalAddress>() << 12), 0x1000))
                   .typed<page_structure::FinalTable>();
    if (pt[pt_offset].get<page_structure::Present>()) {
        debug_log("Already allocated."_sv);
    }
    pt[pt_offset] = page_structure::StructureEntry(page_structure::PhysicalAddress(physical_address.raw_address() >> 12),
                                                   page_structure::Present(true), page_structure::Writable(true));

    return {};
}
}