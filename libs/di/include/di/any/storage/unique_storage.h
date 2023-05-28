#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/any/storage/storage_category.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/object.h>
#include <di/concepts/reference.h>
#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/meta/like_expected.h>
#include <di/meta/list/prelude.h>
#include <di/platform/prelude.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/exchange.h>
#include <di/util/move.h>
#include <di/util/swap.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/unexpected.h>

namespace di::any {
namespace detail {
    template<typename UniqueStorage, concepts::Allocator Alloc>
    struct UniqueStorageManage {
        using Type = Method<UniqueStorageManage, void(This&, Alloc&)>;

        template<typename T>
        void operator()(T&, Alloc&) const;
    };

    template<typename UniqueStorage, concepts::Allocator Alloc>
    constexpr inline auto unique_storage_manage = UniqueStorageManage<UniqueStorage, Alloc> {};
}

template<concepts::Allocator Alloc = platform::DefaultAllocator>
struct UniqueStorage {
public:
    using Manage = meta::Type<detail::UniqueStorageManage<UniqueStorage, Alloc>>;
    using Interface = meta::List<Manage>;

    constexpr static StorageCategory storage_category() { return StorageCategory::TriviallyRelocatable; }

    template<typename T>
    constexpr static bool creation_is_fallible(InPlaceType<T>) {
        return concepts::FallibleAllocator<Alloc>;
    }

    template<typename>
    using CreationResult = meta::AllocatorResult<Alloc>;

    template<typename Any, typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...> && creation_is_fallible(in_place_type<T>))
    constexpr static void create(InPlaceType<Any>, meta::LikeExpected<CreationResult<T>, Any>& self, InPlaceType<T>,
                                 Args&&... args) {
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
    constexpr static auto init(UniqueStorage* self, InPlaceType<T>, Args&&... args) {
        return vocab::as_fallible(di::allocate_one<T>(self->m_allocator)) % [&](T* pointer) {
            util::construct_at(pointer, util::forward<Args>(args)...);
            self->m_pointer = pointer;
        };
    }

    UniqueStorage() = default;

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
            fp(self, self->m_allocator);
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
    [[no_unique_address]] Alloc m_allocator {};
};

namespace detail {
    template<typename UniqueStorage, concepts::Allocator Alloc>
    template<typename T>
    void UniqueStorageManage<UniqueStorage, Alloc>::operator()(T& a, Alloc& allocator) const {
        auto* pointer = util::addressof(a);
        util::destroy_at(pointer);
        di::deallocate_one<T>(allocator, pointer);
    }
}
}
