#pragma once

#include "di/any/concepts/vtable_for.h"
#include "di/any/storage/storage_category.h"
#include "di/meta/algorithm.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/types/prelude.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"
#include "di/util/destroy_at.h"
#include "di/util/exchange.h"
#include "di/util/move.h"

namespace di::any {
namespace detail {
    template<typename InlineStorage>
    struct InlineStorageManage {
        using Type = Method<InlineStorageManage, void(This&, InlineStorage*)>;

        template<typename T>
        void operator()(T&, InlineStorage*) const;
    };

    template<typename InlineStorage>
    constexpr inline auto inline_storage_manage = InlineStorageManage<InlineStorage> {};
}

template<size_t inline_size, size_t inline_align>
struct InlineStorage {
public:
    using Manage = meta::Type<detail::InlineStorageManage<InlineStorage>>;
    using Interface = meta::List<Manage>;

    constexpr static auto storage_category() -> StorageCategory { return StorageCategory::MoveOnly; }

    template<typename T>
    constexpr static auto creation_is_fallible(InPlaceType<T>) -> bool {
        return false;
    }

    template<typename>
    using CreationResult = void;

    constexpr InlineStorage() = default;

    InlineStorage(InlineStorage const&) = delete;
    auto operator=(InlineStorage const&) -> InlineStorage& = delete;

    template<typename T, typename... Args>
    requires(sizeof(T) <= inline_size && alignof(T) <= inline_align && concepts::MoveConstructible<T> &&
             concepts::ConstructibleFrom<T, Args...>)
    constexpr static void init(InlineStorage* self, InPlaceType<T>, Args&&... args) {
        util::construct_at(self->down_cast<T>(), util::forward<Args>(args)...);
    }

    ~InlineStorage() = default;

    constexpr static void move_construct(concepts::VTableFor<Interface> auto& vtable, InlineStorage* dest,
                                         InlineStorage* source) {
        if (!vtable.empty()) {
            auto const fp = vtable[Manage {}];
            fp(dest, source);

            vtable.reset();
        }
    }

    template<concepts::VTableFor<Interface> VTable>
    constexpr static void move_assign(VTable& dest_vtable, InlineStorage* dest, VTable& source_vtable,
                                      InlineStorage* source) {
        destroy(dest_vtable, dest);
        dest_vtable = source_vtable;
        move_construct(source_vtable, dest, source);
    }

    constexpr static void destroy(concepts::VTableFor<Interface> auto& vtable, InlineStorage* self) {
        if (!vtable.empty()) {
            auto const fp = vtable[Manage {}];
            fp(self, nullptr);

            vtable.reset();
        }
    }

    template<typename T>
    auto down_cast() -> T* {
        return static_cast<T*>(address());
    }

    template<typename T>
    auto down_cast() const -> T const* {
        return static_cast<T const*>(address());
    }

private:
    auto address() -> void* { return static_cast<void*>(util::addressof(m_storage[0])); }
    auto address() const -> void const* { return static_cast<void const*>(util::addressof(m_storage[0])); }

    alignas(inline_align) di::Byte m_storage[inline_size];
};

namespace detail {
    template<typename InlineStorage>
    template<typename T>
    void InlineStorageManage<InlineStorage>::operator()(T& a, InlineStorage* b) const {
        if (b) {
            // Move from b into a.
            auto* b_value = b->template down_cast<T>();
            util::construct_at(util::addressof(a), util::move(*b_value));
            util::destroy_at(b_value);
        } else {
            // Just destroy a.
            util::destroy_at(util::addressof(a));
        }
    }
}
}
