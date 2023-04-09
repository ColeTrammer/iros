#pragma once

#include <di/prelude.h>
#include <iris/core/error.h>
#include <iris/core/spinlock.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/region.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
class AddressSpace;

class LockedAddressSpace {
public:
    Expected<void> map_physical_page(VirtualAddress location, PhysicalAddress physical_address, RegionFlags flags);

    Expected<VirtualAddress> allocate_region(di::Box<Region> region);
    Expected<void> allocate_region_at(di::Box<Region> region);

    Expected<void> destroy_region(VirtualAddress start, usize length);

    Expected<void> create_low_identity_mapping(VirtualAddress base, usize page_aligned_length);
    Expected<void> remove_low_identity_mapping(VirtualAddress base, usize page_aligned_length);

    VirtualAddress heap_end() const { return m_heap_end; }
    void set_heap_end(VirtualAddress address) { m_heap_end = address; }

    void invalidate_tlb(VirtualAddress base) { invalidate_tlb(base, 1); }
    void invalidate_tlb(VirtualAddress base, usize byte_length);

    AddressSpace& base();

private:
    di::IntrusiveTreeSet<Region, AddressSpaceRegionListTag> m_regions;
    VirtualAddress m_heap_end { 0 };
};

class AddressSpace
    : public di::Synchronized<LockedAddressSpace, Spinlock>
    , public di::IntrusiveRefCount<AddressSpace> {
    friend class LockedAddressSpace;

public:
    AddressSpace() = default;

    ~AddressSpace();

    PhysicalAddress architecture_page_table_base() const { return m_architecture_page_table_base; }
    void set_architecture_page_table_base(PhysicalAddress value) { m_architecture_page_table_base = value; }

    void set_kernel() { m_kernel = true; }

    void load();

    u64 resident_pages() const { return m_resident_pages.load(di::MemoryOrder::Relaxed); }
    u64 structure_pages() const { return m_structure_pages.load(di::MemoryOrder::Relaxed); }

    Expected<VirtualAddress> allocate_region(usize page_aligned_length, RegionFlags flags);
    Expected<void> allocate_region_at(VirtualAddress location, usize page_aligned_length, RegionFlags flags);

private:
    PhysicalAddress m_architecture_page_table_base { 0 };
    di::Atomic<u64> m_resident_pages { 0 };
    di::Atomic<u64> m_structure_pages { 0 };
    bool m_kernel { false };
};

Expected<void> init_and_load_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                          VirtualAddress kernel_virtual_start,
                                                          PhysicalAddress max_physical_address);

Expected<di::Arc<AddressSpace>> create_empty_user_address_space();
}
