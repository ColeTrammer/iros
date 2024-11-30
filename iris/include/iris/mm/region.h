#pragma once

#include <di/assert/prelude.h>
#include <di/container/intrusive/prelude.h>
#include <iris/mm/backing_object.h>
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
    Region() = default;

    constexpr explicit Region(VirtualAddress base, usize length, RegionFlags flags)
        : m_base(base), m_length(length), m_flags(flags) {
        ASSERT_GT(length, 0U);
        ASSERT_EQ(length % 0x1000, 0U);
    }

    constexpr auto base() const -> VirtualAddress { return m_base; }
    constexpr auto end() const -> VirtualAddress { return m_base + length(); }
    constexpr auto length() const -> usize { return m_length; }
    constexpr auto pages() const -> usize { return length() / 0x1000; }

    constexpr auto each_page() const { return di::iota(base(), end()) | di::stride(0x1000z); }

    constexpr void set_base(VirtualAddress base) { m_base = base; }
    constexpr void set_length(usize length) { m_length = length; }
    constexpr void set_flags(RegionFlags flags) { m_flags = flags; }
    constexpr void set_backing_object(di::Arc<BackingObject> backing_object) {
        m_backing_object = di::move(backing_object);
    }
    constexpr void set_end(VirtualAddress end) { m_length = end - m_base; }

    constexpr auto flags() const -> RegionFlags { return m_flags; }
    constexpr auto backing_object() const -> BackingObject& { return *m_backing_object; }

    constexpr auto readable() const -> bool { return !!(m_flags & RegionFlags::Readable); }
    constexpr auto writable() const -> bool { return !!(m_flags & RegionFlags::Writable); }
    constexpr auto executable() const -> bool { return !!(m_flags & RegionFlags::Executable); }
    constexpr auto user() const -> bool { return !!(m_flags & RegionFlags::User); }

    constexpr auto contains(VirtualAddress b) const -> bool { return b >= base() && b < end(); }

    constexpr auto compare_with_address(VirtualAddress b) const -> di::strong_ordering {
        if (contains(b)) {
            return di::strong_ordering::equal;
        }
        if (base() < b) {
            return di::strong_ordering::less;
        }
        return di::strong_ordering::greater;
    }

private:
    constexpr friend auto operator==(Region const& a, Region const& b) -> bool { return a.base() == b.base(); }
    constexpr friend auto operator<=>(Region const& a, Region const& b) -> di::strong_ordering {
        // We assume that Region object which are compared are not overlapping.
        // This invariant is ensured by the AddressSpace object, which internally
        // uses this comparison function in its di::TreeSet<> of Regions.
        return a.base() <=> b.base();
    }

    // There overloads are provided to enable heterogenous lookup of Region objects in a
    // di::TreeSet by a VirtualAddress.
    constexpr friend auto operator==(Region const& a, VirtualAddress b) -> bool { return a.contains(b); }
    constexpr friend auto operator<=>(Region const& a, VirtualAddress b) -> di::strong_ordering {
        return a.compare_with_address(b);
    }

    VirtualAddress m_base;
    usize m_length { 0 };
    di::Arc<BackingObject> m_backing_object;
    RegionFlags m_flags {};
};

constexpr void AddressSpaceRegionListTag::did_remove(auto&, auto& node) {
    auto to_drop = di::Box<Region>(di::addressof(node));
}
}
