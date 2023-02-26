#pragma once

#include <di/prelude.h>
#include <iris/core/error.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/region.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
class AddressSpace : public di::IntrusiveRefCount<AddressSpace> {
public:
    AddressSpace() = default;

    ~AddressSpace();

    PhysicalAddress architecture_page_table_base() const { return m_architecture_page_table_base; }
    void set_architecture_page_table_base(PhysicalAddress value) { m_architecture_page_table_base = value; }

    void set_kernel() { m_kernel = true; }

    void load();

    Expected<void> map_physical_page(VirtualAddress location, PhysicalAddress physical_address, RegionFlags flags);

    Expected<VirtualAddress> allocate_region(usize page_aligned_length, RegionFlags flags);
    Expected<void> allocate_region_at(VirtualAddress location, usize page_aligned_length, RegionFlags flags);

    u64 resident_pages() const { return m_resident_pages; }
    u64 structure_pages() const { return m_structure_pages; }

private:
    PhysicalAddress m_architecture_page_table_base { 0 };
    u64 m_resident_pages { 0 };
    u64 m_structure_pages { 0 };
    di::TreeSet<Region> m_regions;
    bool m_kernel { false };
};

Expected<void> init_and_load_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                          VirtualAddress kernel_virtual_start,
                                                          PhysicalAddress max_physical_address);

Expected<di::Arc<AddressSpace>> create_empty_user_address_space();
}
