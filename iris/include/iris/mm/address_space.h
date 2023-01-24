#pragma once

#include <di/prelude.h>
#include <iris/core/error.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/region.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
class AddressSpace {
public:
    explicit AddressSpace(u64 architecture_page_table_base) : m_architecture_page_table_base(architecture_page_table_base) {}

    u64 architecture_page_table_base() const { return m_architecture_page_table_base; }

    Expected<void> map_physical_page(VirtualAddress location, PhysicalAddress physical_address);

    Expected<VirtualAddress> allocate_region(usize page_aligned_length);
    Expected<void> allocate_region_at(VirtualAddress location, usize page_aligned_length);

private:
    u64 m_architecture_page_table_base;
    di::TreeSet<Region> m_regions;
};
}