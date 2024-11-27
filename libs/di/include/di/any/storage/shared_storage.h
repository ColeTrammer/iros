#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/any/storage/storage_category.h>
#include <di/container/algorithm/max.h>
#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/meta/algorithm.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/platform/prelude.h>
#include <di/sync/atomic.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/exchange.h>
#include <di/util/immovable.h>
#include <di/util/move.h>
#include <di/util/swap.h>
#include <di/vocab/expected/as_fallible.h>
#include <di/vocab/expected/try_infallible.h>

namespace di::any {
namespace detail {
    enum class OpForConsteval {
        Destroy,
        RefInc,
        RefDec,
    };

    template<typename SharedStorage, concepts::Allocator Alloc>
    struct SharedStorageManage {
        using Type = Method<SharedStorageManage, void(This&, SharedStorage*, Alloc&, OpForConsteval)>;

        template<typename T>
        constexpr void operator()(T&, SharedStorage*, Alloc&, OpForConsteval) const;
    };

    template<typename SharedStorage, concepts::Allocator Alloc>
    constexpr inline auto shared_storage_manage = SharedStorageManage<SharedStorage, Alloc> {};

    struct RefCount {
        sync::Atomic<usize> ref_count { 1 };
    };

    template<typename T>
    struct ObjectWithRefCount : RefCount {
        ObjectWithRefCount(ObjectWithRefCount const&) = delete;
        ObjectWithRefCount(ObjectWithRefCount&&) = delete;

        template<typename... Args>
        constexpr ObjectWithRefCount(Args&&... args) : object(util::forward<Args>(args)...) {}

        constexpr auto to_object_pointer() -> T* { return util::addressof(object); }

        T object;
    };
}

template<concepts::Allocator Alloc = platform::DefaultAllocator>
struct SharedStorage {
private:
    template<typename, concepts::Allocator>
    friend struct detail::SharedStorageManage;

public:
    using Manage = meta::Type<detail::SharedStorageManage<SharedStorage, Alloc>>;
    using Interface = meta::List<Manage>;

    constexpr static auto storage_category() -> StorageCategory { return StorageCategory::Copyable; }

    template<typename T>
    constexpr static auto creation_is_fallible(InPlaceType<T>) -> bool {
        return concepts::FallibleAllocator<Alloc>;
    }

    template<typename>
    using CreationResult = meta::AllocatorResult<Alloc>;

    template<typename Any, typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...> && creation_is_fallible(in_place_type<T>))
    constexpr static void create(InPlaceType<Any>, meta::LikeExpected<CreationResult<T>, Any>& self, InPlaceType<T>,
                                 Args&&... args) {
        using Store = detail::ObjectWithRefCount<T>;
        auto result = di::allocate_one<Store>(self->m_allocator);
        if (!result) {
            self = vocab::Unexpected(util::move(result).error());
            return;
        }

        auto* pointer = *result;
        util::construct_at(pointer, util::forward<Args>(args)...);

        self->m_pointer = pointer;
    }

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...> && alignof(T) <= alignof(usize))
    constexpr static auto init(SharedStorage* self, InPlaceType<T>, Args&&... args) {
        using Store = detail::ObjectWithRefCount<T>;
        return vocab::as_fallible(di::allocate_one<Store>(self->m_allocator)) % [&](Store* pointer) {
            util::construct_at(pointer, util::forward<Args>(args)...);
            self->m_pointer = pointer;
        } | vocab::try_infallible;
    }

    SharedStorage() = default;

    SharedStorage(SharedStorage const&) = default;
    auto operator=(SharedStorage const&) -> SharedStorage& = default;

    ~SharedStorage() = default;

    constexpr static void copy_construct(concepts::VTableFor<Interface> auto const& vtable, SharedStorage* dest,
                                         SharedStorage const* source) {
        if ((dest->m_pointer = source->m_pointer)) {
            if consteval {
                auto const fp = vtable[Manage {}];
                fp(dest, dest, dest->m_allocator, detail::OpForConsteval::RefInc);
            } else {
                dest->fetch_add_ref_count();
            }
        }
    }

    constexpr static void move_construct(concepts::VTableFor<Interface> auto& vtable, SharedStorage* dest,
                                         SharedStorage* source) {
        dest->m_pointer = util::exchange(source->m_pointer, nullptr);
        vtable.reset();
    }

    template<concepts::VTableFor<Interface> VTable>
    constexpr static void copy_assign(VTable& dest_vtable, SharedStorage* dest, VTable const& source_vtable,
                                      SharedStorage const* source) {
        destroy(dest_vtable, dest);
        dest_vtable = source_vtable;
        copy_construct(source_vtable, dest, source);
    }

    template<concepts::VTableFor<Interface> VTable>
    constexpr static void move_assign(VTable& dest_vtable, SharedStorage* dest, VTable& source_vtable,
                                      SharedStorage* source) {
        destroy(dest_vtable, dest);
        dest_vtable = source_vtable;
        move_construct(source_vtable, dest, source);
    }

    constexpr static void destroy(concepts::VTableFor<Interface> auto& vtable, SharedStorage* self) {
        if (self->m_pointer) {
            auto const fp = vtable[Manage {}];
            if consteval {
                fp(self, self, self->m_allocator, detail::OpForConsteval::RefDec);
            } else {
                if (self->fetch_sub_ref_count() == 1) {
                    fp(self, self, self->m_allocator, detail::OpForConsteval::Destroy);
                }
            }
            self->m_pointer = nullptr;
        }
    }

    template<typename T>
    constexpr auto down_cast() -> T* {
        return static_cast<detail::ObjectWithRefCount<T>*>(m_pointer)->to_object_pointer();
    }

    template<typename T>
    constexpr auto down_cast() const -> T const* {
        return static_cast<detail::ObjectWithRefCount<T> const*>(m_pointer)->to_object_pointer();
    }

private:
    constexpr explicit SharedStorage(void* pointer) : m_pointer(pointer) {}

    constexpr auto fetch_sub_ref_count() -> usize {
        auto& ref_count = static_cast<detail::RefCount*>(m_pointer)->ref_count;
        return ref_count.fetch_sub(1, sync::MemoryOrder::Relaxed);
    }

    constexpr auto fetch_add_ref_count() -> usize {
        auto& ref_count = static_cast<detail::RefCount*>(m_pointer)->ref_count;
        return ref_count.fetch_add(1, sync::MemoryOrder::AcquireRelease);
    }

    void* m_pointer { nullptr };
    [[no_unique_address]] Alloc m_allocator {};
};

namespace detail {
    template<typename SharedStorage, concepts::Allocator Alloc>
    template<typename T>
    constexpr void SharedStorageManage<SharedStorage, Alloc>::operator()(T&, SharedStorage* storage, Alloc& allocator,
                                                                         OpForConsteval op) const {
        auto* pointer = static_cast<ObjectWithRefCount<T>*>(storage->m_pointer);
        if consteval {
            switch (op) {
                case OpForConsteval::RefInc:
                    pointer->ref_count.fetch_add(1);
                    return;
                case OpForConsteval::RefDec:
                    if (pointer->ref_count.fetch_sub(1) == 1) {
                        break;
                    }
                    return;
                default:
                    return;
            }
        }

        util::destroy_at(pointer);
        di::deallocate_one<detail::ObjectWithRefCount<T>>(allocator, pointer);
    }
}
}
