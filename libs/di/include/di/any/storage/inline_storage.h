#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/any/storage/storage_category.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/object.h>
#include <di/concepts/reference.h>
#include <di/meta/list/prelude.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/move.h>

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

    constexpr static StorageCategory storage_category() { return StorageCategory::MoveOnly; }

    template<typename T>
    constexpr static bool creation_is_fallible(InPlaceType<T>) {
        return false;
    }

    constexpr InlineStorage() {}

    InlineStorage(InlineStorage const&) = delete;
    InlineStorage& operator=(InlineStorage const&) = delete;

    template<typename T, typename... Args>
    requires(sizeof(T) <= inline_size && alignof(T) <= inline_align && concepts::ConstructibleFrom<T, Args...>)
    constexpr InlineStorage(InPlaceType<T>, Args&&... args) {
        util::construct_at(down_cast<T>(), util::forward<Args>(args)...);
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
    T* down_cast() {
        return static_cast<T*>(address());
    }

    template<typename T>
    T const* down_cast() const {
        return static_cast<T const*>(address());
    }

private:
    void* address() { return static_cast<void*>(util::addressof(m_storage[0])); }
    void const* address() const { return static_cast<void const*>(util::addressof(m_storage[0])); }

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