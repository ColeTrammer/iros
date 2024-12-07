#pragma once

#include "di/any/concepts/vtable_for.h"
#include "di/any/storage/storage_category.h"
#include "di/assert/assert_bool.h"
#include "di/container/allocator/allocate_one.h"
#include "di/container/allocator/allocator.h"
#include "di/container/allocator/deallocate_one.h"
#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/vocab.h"
#include "di/platform/prelude.h"
#include "di/types/prelude.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"
#include "di/util/destroy_at.h"
#include "di/util/exchange.h"
#include "di/util/move.h"
#include "di/vocab/expected/as_fallible.h"
#include "di/vocab/expected/try_infallible.h"

namespace di::any {
namespace detail {
    template<typename HybridStorage, concepts::Allocator Alloc>
    struct HybridStorageManage {
        using Type = Method<HybridStorageManage, void(This&, HybridStorage*, HybridStorage*, Alloc&)>;

        template<typename T>
        constexpr void operator()(T&, HybridStorage*, HybridStorage*, Alloc&) const;
    };

    template<typename HybridStorage, concepts::Allocator Alloc>
    constexpr inline auto hybrid_storage_manage = HybridStorageManage<HybridStorage, Alloc> {};
}

template<StorageCategory category = StorageCategory::MoveOnly, size_t inline_size = 2 * sizeof(void*),
         size_t inline_align = alignof(void*), concepts::Allocator Alloc = platform::DefaultAllocator>
struct HybridStorage {
    static_assert(category == StorageCategory::MoveOnly || category == StorageCategory::Immovable,
                  "HybridStorage only supports MoveOnly and Immovable objects");

public:
    using Manage = meta::Type<detail::HybridStorageManage<HybridStorage, Alloc>>;
    using Interface = meta::List<Manage>;

    template<typename, concepts::Allocator>
    friend struct detail::HybridStorageManage;

    constexpr static auto storage_category() -> StorageCategory { return category; }

    template<typename T>
    constexpr static auto creation_is_inline(InPlaceType<T>) -> bool {
        return sizeof(T) <= inline_size && alignof(T) <= inline_align &&
               (concepts::MoveConstructible<T> || category == StorageCategory::Immovable);
    }

    template<typename T>
    constexpr static auto creation_is_fallible(InPlaceType<T>) -> bool {
        return !creation_is_inline(in_place_type<T>) && concepts::FallibleAllocator<Alloc>;
    }

    template<typename T>
    using CreationResult =
        meta::Conditional<creation_is_fallible(in_place_type<T>), meta::AllocatorResult<Alloc>, void>;

    HybridStorage() = default;

    HybridStorage(HybridStorage const&) = delete;
    auto operator=(HybridStorage const&) -> HybridStorage& = delete;

    template<typename Any, typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...> && creation_is_fallible(in_place_type<T>))
    constexpr static void create(InPlaceType<Any>, meta::LikeExpected<CreationResult<T>, Any>& self, InPlaceType<T>,
                                 Args&&... args) {
        if consteval {
            self->m_pointer = ::new T(di::forward<Args>(args)...);
            return;
        }

        auto result = di::allocate_one<T>(self->m_allocator);
        if (!result) {
            self = vocab::Unexpected(util::move(result).error());
            return;
        }

        auto* pointer = *result;
        util::construct_at(pointer, util::forward<Args>(args)...);

        self->m_pointer = pointer;
    }

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr static auto init(HybridStorage* self, InPlaceType<T>, Args&&... args) {
        if consteval {
            self->m_pointer = ::new T(di::forward<Args>(args)...);
            if constexpr (concepts::FallibleAllocator<Alloc> && !creation_is_inline(in_place_type<T>)) {
                return meta::AllocatorResult<Alloc> {};
            } else {
                return;
            }
        }
        if constexpr (!creation_is_inline(in_place_type<T>)) {
            return vocab::as_fallible(di::allocate_one<T>(self->m_allocator)) % [&](T* pointer) {
                util::construct_at(pointer, util::forward<Args>(args)...);
                self->m_pointer = pointer;
            } | vocab::try_infallible;
        } else {
            util::construct_at(self->down_cast<T>(), util::forward<Args>(args)...);
        }
    }

    ~HybridStorage() = default;

    constexpr static void move_construct(concepts::VTableFor<Interface> auto& vtable, HybridStorage* dest,
                                         HybridStorage* source)
    requires(category == StorageCategory::MoveOnly)
    {
        if (!vtable.empty()) {
            auto const fp = vtable[Manage {}];
            fp(source, dest, source, dest->m_allocator);

            vtable.reset();
        }
    }

    template<concepts::VTableFor<Interface> VTable>
    constexpr static void move_assign(VTable& dest_vtable, HybridStorage* dest, VTable& source_vtable,
                                      HybridStorage* source)
    requires(category == StorageCategory::MoveOnly)
    {
        destroy(dest_vtable, dest);
        dest_vtable = source_vtable;
        move_construct(source_vtable, dest, source);
    }

    constexpr static void destroy(concepts::VTableFor<Interface> auto& vtable, HybridStorage* self) {
        if (!vtable.empty()) {
            auto const fp = vtable[Manage {}];
            fp(self, self, nullptr, self->m_allocator);

            vtable.reset();
        }
    }

    template<typename T>
    constexpr auto down_cast() -> T* {
        if consteval {
            return static_cast<T*>(m_pointer);
        }
        if constexpr (!creation_is_inline(in_place_type<T>)) {
            return static_cast<T*>(m_pointer);
        } else {
            return static_cast<T*>(address());
        }
    }

    template<typename T>
    constexpr auto down_cast() const -> T const* {
        if consteval {
            return static_cast<T const*>(m_pointer);
        }
        if constexpr (!creation_is_inline(in_place_type<T>)) {
            return static_cast<T const*>(m_pointer);
        } else {
            return static_cast<T const*>(address());
        }
    }

private:
    auto address() -> void* { return static_cast<void*>(util::addressof(m_storage[0])); }
    auto address() const -> void const* { return static_cast<void const*>(util::addressof(m_storage[0])); }

    union {
        void* m_pointer { nullptr };
        alignas(inline_align) byte m_storage[inline_size];
    };
    [[no_unique_address]] Alloc m_allocator {};
};

namespace detail {
    template<typename HybridStorage, concepts::Allocator Alloc>
    template<typename T>
    constexpr void HybridStorageManage<HybridStorage, Alloc>::operator()(T&, HybridStorage* as, HybridStorage* bs,
                                                                         Alloc& allocator) const {
        if consteval {
            if (bs) {
                if constexpr (HybridStorage::storage_category() == StorageCategory::MoveOnly) {
                    // Move from b into a.
                    as->m_pointer = util::exchange(bs->m_pointer, nullptr);
                } else {
                    DI_ASSERT(!bs);
                }
            } else {
                // Just destroy a.
                auto* pointer = util::exchange(as->m_pointer, nullptr);
                auto* a_value = static_cast<T*>(pointer);
                ::delete a_value;
            }
            return;
        }
        if constexpr (!HybridStorage::creation_is_inline(in_place_type<T>)) {
            if (bs) {
                if constexpr (HybridStorage::storage_category() == StorageCategory::MoveOnly) {
                    // Move from b into a.
                    as->m_pointer = util::exchange(bs->m_pointer, nullptr);
                } else {
                    DI_ASSERT(!bs);
                }
            } else {
                // Just destroy a.
                auto* pointer = util::exchange(as->m_pointer, nullptr);
                auto* a_value = static_cast<T*>(pointer);
                util::destroy_at(a_value);
                di::deallocate_one<T>(allocator, a_value);
            }
        } else {
            if (bs) {
                if constexpr (HybridStorage::storage_category() == StorageCategory::MoveOnly) {
                    // Move from b into a.
                    auto* b_value = bs->template down_cast<T>();
                    auto* a_value = as->template down_cast<T>();
                    util::construct_at(a_value, util::move(*b_value));
                    util::destroy_at(b_value);
                } else {
                    DI_ASSERT(!bs);
                }
            } else {
                // Just destroy a.
                auto* a_value = as->template down_cast<T>();
                util::destroy_at(a_value);
            }
        }
    }
}
}
