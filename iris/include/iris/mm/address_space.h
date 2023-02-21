#pragma once

#include <di/prelude.h>
#include <iris/core/error.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/region.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
class AddressSpace : public di::IntrusiveRefCount<AddressSpace> {
public:
    constexpr explicit AddressSpace(u64 architecture_page_table_base, bool kernel)
        : m_architecture_page_table_base(architecture_page_table_base), m_kernel(kernel) {}

    u64 architecture_page_table_base() const { return m_architecture_page_table_base; }
    void set_architecture_page_table_base(u64 value) { m_architecture_page_table_base = value; }

    void load();

    Expected<void> map_physical_page(VirtualAddress location, PhysicalAddress physical_address, RegionFlags flags);

    Expected<VirtualAddress> allocate_region(usize page_aligned_length, RegionFlags flags);
    Expected<void> allocate_region_at(VirtualAddress location, usize page_aligned_length, RegionFlags flags);

private:
    u64 m_architecture_page_table_base { 0 };
    di::TreeSet<Region> m_regions;
    bool m_kernel { false };
};

Expected<void> init_and_load_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                          VirtualAddress kernel_virtual_start,
                                                          PhysicalAddress max_physical_address);

Expected<di::Arc<AddressSpace>> create_empty_user_address_space();
}
