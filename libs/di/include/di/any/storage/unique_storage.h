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
#include <di/util/exchange.h>
#include <di/util/move.h>
#include <di/util/swap.h>
#include <di/vocab/error/prelude.h>

namespace di::any {
namespace detail {
    template<typename UniqueStorage>
    struct UniqueStorageManage {
        using Type = Method<UniqueStorageManage, void(This&)>;

        template<typename T>
        void operator()(T&) const;
    };

    template<typename UniqueStorage>
    constexpr inline auto unique_storage_manage = UniqueStorageManage<UniqueStorage> {};
}

struct UniqueStorage {
public:
    using Manage = meta::Type<detail::UniqueStorageManage<UniqueStorage>>;
    using Interface = meta::List<Manage>;

    constexpr static StorageCategory storage_category() { return StorageCategory::TriviallyRelocatable; }

    template<typename T>
    constexpr static bool creation_is_fallible(InPlaceType<T>) {
        return true;
    }

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr static auto init(UniqueStorage* self, InPlaceType<T>, Args&&... args) {
        return platform::DefaultFallibleAllocator<T>().allocate(1) % [&](container::Allocation<T> result) {
            util::construct_at(result.data, util::forward<Args>(args)...);
            self->m_pointer = result.data;
        };
    }

    constexpr UniqueStorage() {}

    UniqueStorage(UniqueStorage const&) = default;
    UniqueStorage& operator=(UniqueStorage const&) = default;

    ~UniqueStorage() = default;

    constexpr static void move_construct(concepts::VTableFor<Interface> auto& vtable, UniqueStorage* dest,
                                         UniqueStorage* source) {
        dest->m_pointer = util::exchange(source->m_pointer, nullptr);
        vtable.reset();
    }

    template<concepts::VTableFor<Interface> VTable>
    constexpr static void move_assign(VTable& dest_vtable, UniqueStorage* dest, VTable& source_vtable,
                                      UniqueStorage* source) {
        destroy(dest_vtable, dest);
        dest_vtable = source_vtable;
        move_construct(source_vtable, dest, source);
    }

    constexpr static void destroy(concepts::VTableFor<Interface> auto& vtable, UniqueStorage* self) {
        if (self->m_pointer) {
            auto const fp = vtable[Manage {}];
            fp(self);
            self->m_pointer = nullptr;
        }
    }

    template<typename T>
    T* down_cast() {
        return static_cast<T*>(m_pointer);
    }

    template<typename T>
    T const* down_cast() const {
        return static_cast<T const*>(m_pointer);
    }

private:
    constexpr explicit UniqueStorage(void* pointer) : m_pointer(pointer) {}

    void* m_pointer { nullptr };
};

namespace detail {
    template<typename UniqueStorage>
    template<typename T>
    void UniqueStorageManage<UniqueStorage>::operator()(T& a) const {
        auto* pointer = util::addressof(a);
        util::destroy_at(pointer);
        platform::DefaultFallibleAllocator<T>().deallocate(pointer, 1);
    }
}
}
