#pragma once

#include <di/assert/prelude.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
enum class RegionFlags {
    None = 0,
    Readable = 1,
    Writable = 2,
    Executable = 4,
    User = 8,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(RegionFlags)

class Region;

struct AddressSpaceRegionListTag : di::container::IntrusiveTagBase<Region> {
    constexpr static void did_remove(auto&, auto& node);
};

class Region : public di::IntrusiveTreeSetNode<AddressSpaceRegionListTag> {
public:
    constexpr explicit Region(VirtualAddress base, usize length, RegionFlags flags)
        : m_base(base), m_length(length), m_flags(flags) {
        ASSERT_GT(length, 0u);
        ASSERT_EQ(length % 0x1000, 0u);
    }

    constexpr VirtualAddress base() const { return m_base; }
    constexpr VirtualAddress end() const { return m_base + length(); }
    constexpr usize length() const { return m_length; }
    constexpr usize pages() const { return length() / 0x1000; }

    constexpr auto each_page() const { return di::iota(base(), end()) | di::stride(0x1000z); }

    constexpr void set_base(VirtualAddress base) { m_base = base; }

    constexpr RegionFlags flags() const { return m_flags; }

    constexpr bool readable() const { return !!(m_flags & RegionFlags::Readable); }
    constexpr bool writable() const { return !!(m_flags & RegionFlags::Writable); }
    constexpr bool executable() const { return !!(m_flags & RegionFlags::Executable); }
    constexpr bool user() const { return !!(m_flags & RegionFlags::User); }

    constexpr bool contains(VirtualAddress b) const { return b >= base() && b < end(); }

    constexpr di::strong_ordering compare_with_address(VirtualAddress b) const {
        if (contains(b)) {
            return di::strong_ordering::equal;
        }
        if (base() < b) {
            return di::strong_ordering::less;
        }
        return di::strong_ordering::greater;
    }

private:
    constexpr friend bool operator==(Region const& a, Region const& b) { return a.base() == b.base(); }
    constexpr friend di::strong_ordering operator<=>(Region const& a, Region const& b) {
        // We assume that Region object which are compared are not overlapping.
        // This invariant is ensured by the AddressSpace object, which internally
        // uses this comparison function in its di::TreeSet<> of Regions.
        return a.base() <=> b.base();
    }

    // There overloads are provided to enable heterogenous lookup of Region objects in a
    // di::TreeSet by a VirtualAddress.
    constexpr friend bool operator==(Region const& a, VirtualAddress b) { return a.contains(b); }
    constexpr friend di::strong_ordering operator<=>(Region const& a, VirtualAddress b) {
        return a.compare_with_address(b);
    }

    VirtualAddress m_base;
    usize m_length { 0 };
    RegionFlags m_flags {};
};

constexpr void AddressSpaceRegionListTag::did_remove(auto&, auto& node) {
    di::destroy_at(di::addressof(node));
    di::platform::DefaultFallibleAllocator<Region>().deallocate(di::addressof(node), 1);
}
}
