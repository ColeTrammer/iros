#pragma once

#include <di/assert/prelude.h>
#include <di/container/intrusive/prelude.h>
#include <di/function/prelude.h>
#include <di/meta/trivial.h>
#include <di/sync/memory_order.h>
#include <di/types/integers.h>
#include <di/types/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/virtual_address.h>

namespace iris::mm {
/// @brief A physical page of memory used for the page tables themselves.
struct PageStructurePhysicalPage : di::IntrusiveListNode<> {
    struct Parent {};
    struct Leaf {};

    constexpr explicit PageStructurePhysicalPage(Parent) : children() {}
    constexpr explicit PageStructurePhysicalPage(Leaf) : mapped_page_count(0) {}

    union {
        di::IntrusiveList<PageStructurePhysicalPage> children;
        u64 mapped_page_count;
    };
};

struct BackedPhysicalPage;

struct BackedPhysicalPagePtrTag {
private:
    friend void tag_invoke(di::Tag<di::vocab::intrusive_ptr_increment>, di::InPlaceType<BackedPhysicalPagePtrTag>,
                           BackedPhysicalPage* ptr);
    friend void tag_invoke(di::Tag<di::vocab::intrusive_ptr_decrement>, di::InPlaceType<BackedPhysicalPagePtrTag>,
                           BackedPhysicalPage* ptr);
};

struct BackedPhysicalPageTreeTag : di::container::IntrusiveTagBase<BackedPhysicalPage> {
    static void did_remove(auto&, auto& node);
};

/// @brief A physical page of memory tracked by a backing object.
struct BackedPhysicalPage : di::IntrusiveTreeSetNode<BackedPhysicalPageTreeTag> {
    constexpr explicit BackedPhysicalPage(u64 page_number) : page_number(page_number) {}

    di::sync::Atomic<usize> reference_count { 1 };
    u64 page_number;

private:
    constexpr friend bool operator==(BackedPhysicalPage const& a, BackedPhysicalPage const& b) {
        return a.page_number == b.page_number;
    }
    constexpr friend di::strong_ordering operator<=>(BackedPhysicalPage const& a, BackedPhysicalPage const& b) {
        return a.page_number <=> b.page_number;
    }

    constexpr friend bool operator==(BackedPhysicalPage const& a, u64 b) { return a.page_number == b; }
    constexpr friend di::strong_ordering operator<=>(BackedPhysicalPage const& a, u64 b) { return a.page_number <=> b; }
};

/// @brief A physical page of memory.
///
/// The kernel pre-allocates one of these for every physical page of memory in the system. This ensures that pages can
/// be tracked correctly, and prevents circular dependencies between the page frame allocator and the kernel heap. As a
/// consequence, this structure must be kept as small as possible.
struct PhysicalPage {
    union {
        PageStructurePhysicalPage as_page_structure_page;
        BackedPhysicalPage as_backed_page;
    };
};

constexpr inline auto physical_page_base =
    VirtualAddress(0xFFFF800000000000_u64 + 4096_u64 * 512_u64 * 512_u64 * 512_u64);

namespace detail {
    struct PhysicalAddressFunction {
        inline PhysicalAddress operator()(PhysicalPage const& page) const {
            return void_pointer_to_physical_address(&page);
        }

        inline PhysicalAddress operator()(BackedPhysicalPage const& page) const {
            return void_pointer_to_physical_address(&page);
        }

        inline PhysicalAddress operator()(PageStructurePhysicalPage const& page) const {
            return void_pointer_to_physical_address(&page);
        }

    private:
        static inline PhysicalAddress void_pointer_to_physical_address(void const* pointer) {
            auto page_number = (VirtualAddress(di::to_uintptr(pointer)) - physical_page_base) / sizeof(PhysicalPage);
            return PhysicalAddress(page_number * 4096);
        }
    };
}

constexpr inline auto physical_address = detail::PhysicalAddressFunction {};

inline PhysicalPage& physical_page(PhysicalAddress address) {
    ASSERT(address.raw_value() % 4096 == 0);
    auto const page_number = address.raw_value() / 4096;
    return *(reinterpret_cast<PhysicalPage*>(physical_page_base.raw_value()) + page_number);
}

inline PageStructurePhysicalPage& page_structure_page(PhysicalAddress address) {
    return physical_page(address).as_page_structure_page;
}

inline BackedPhysicalPage& backed_page(PhysicalAddress address) {
    return physical_page(address).as_backed_page;
}

namespace detail {
    struct BumpPage {
        inline void operator()(BackedPhysicalPage& page) const {
            page.reference_count.fetch_add(1, di::sync::MemoryOrder::AcquireRelease);
        }

        inline void operator()(PhysicalAddress address) const {
            auto& page = backed_page(address);
            page.reference_count.fetch_add(1, di::sync::MemoryOrder::AcquireRelease);
        }
    };
}

constexpr inline auto bump_page = detail::BumpPage {};

namespace detail {
    struct DropPageFunction {
        inline void operator()(BackedPhysicalPage& page) const {
            if (page.reference_count.fetch_sub(1, di::sync::MemoryOrder::AcquireRelease) == 1) {
                deallocate_page_frame(physical_address(page));
            }
        }

        inline void operator()(PhysicalAddress address) const {
            auto& page = backed_page(address);
            if (page.reference_count.fetch_sub(1, di::sync::MemoryOrder::AcquireRelease) == 1) {
                deallocate_page_frame(physical_address(page));
            }
        }
    };
}

constexpr inline auto drop_page = iris::mm::detail::DropPageFunction {};

void BackedPhysicalPageTreeTag::did_remove(auto&, auto& node) {
    drop_page(node);
}
}
