#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/sync/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/core/error.h>
#include <iris/core/interruptible_spinlock.h>
#include <iris/mm/backing_object.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/physical_page.h>
#include <iris/mm/region.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
class AddressSpace;

class LockedAddressSpace {
public:
    auto map_physical_page_early(VirtualAddress location, PhysicalAddress physical_address, RegionFlags flags)
        -> Expected<void>;
    auto map_physical_page(VirtualAddress location, PhysicalAddress physical_address, RegionFlags flags)
        -> Expected<void>;

    auto allocate_region(di::Arc<BackingObject> backing_object, di::Box<Region> region) -> Expected<VirtualAddress>;
    auto allocate_region_at(di::Arc<BackingObject> backing_object, di::Box<Region> region) -> Expected<void>;

    auto destroy_region(VirtualAddress start, usize length) -> Expected<void>;

    auto create_low_identity_mapping(VirtualAddress base, usize page_aligned_length) -> Expected<void>;
    auto remove_low_identity_mapping(VirtualAddress base, usize page_aligned_length) -> Expected<void>;

    auto setup_physical_memory_map(PhysicalAddress start, PhysicalAddress end, VirtualAddress virtual_start)
        -> Expected<void>;
    auto setup_kernel_region(PhysicalAddress kernel_physical_start, VirtualAddress kernel_virtual_start,
                             VirtualAddress kernel_virtual_end, RegionFlags flags) -> Expected<void>;

    auto bootstrap_kernel_page_tracking() -> Expected<void>;

    void flush_tlb_global(VirtualAddress base) { flush_tlb_global(base, 1); }
    void flush_tlb_global(VirtualAddress base, usize byte_length);

    auto base() -> AddressSpace&;

private:
    friend class AddressSpace;

    di::IntrusiveTreeSet<Region, AddressSpaceRegionListTag> m_regions;
};

class AddressSpace
    : public di::Synchronized<LockedAddressSpace, InterruptibleSpinlock>
    , public di::IntrusiveRefCount<AddressSpace> {
    friend class LockedAddressSpace;

public:
    AddressSpace() = default;

    ~AddressSpace();

    auto architecture_page_table_base() const -> PhysicalAddress { return m_architecture_page_table_base; }
    void set_architecture_page_table_base(PhysicalAddress value) { m_architecture_page_table_base = value; }

    void set_kernel() { m_kernel = true; }
    auto is_kernel() const -> bool { return m_kernel; }

    void load();

    auto resident_pages() const -> u64 { return m_resident_pages.load(di::MemoryOrder::Relaxed); }
    auto structure_pages() const -> u64 { return m_structure_pages.load(di::MemoryOrder::Relaxed); }

    auto allocate_region(di::Arc<BackingObject> backing_object, usize page_aligned_length, RegionFlags flags)
        -> Expected<VirtualAddress>;
    auto allocate_region_at(di::Arc<BackingObject> backing_object, VirtualAddress location, usize page_aligned_length,
                            RegionFlags flags) -> Expected<void>;

private:
    PhysicalAddress m_architecture_page_table_base { 0 };
    di::Atomic<u64> m_resident_pages { 0 };
    di::Atomic<u64> m_structure_pages { 0 };
    bool m_kernel { false };
};

auto init_and_load_initial_kernel_address_space(PhysicalAddress kernel_physical_start,
                                                VirtualAddress kernel_virtual_start,
                                                PhysicalAddress max_physical_address) -> Expected<void>;

auto create_empty_user_address_space() -> Expected<di::Arc<AddressSpace>>;
}
