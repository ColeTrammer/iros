#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/any/storage/storage_category.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/object.h>
#include <di/concepts/reference.h>
#include <di/container/algorithm/max.h>
#include <di/meta/list/prelude.h>
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
#include <di/vocab/error/prelude.h>

namespace di::any {
namespace detail {
    template<typename SharedStorage>
    struct SharedStorageManage {
        using Type = Method<SharedStorageManage, void(This&)>;

        template<typename T>
        void operator()(T&) const;
    };

    template<typename SharedStorage>
    constexpr inline auto shared_storage_manage = SharedStorageManage<SharedStorage> {};

    template<typename T>
    struct ObjectWithRefCount : util::Immovable {
    public:
        template<typename... Args>
        constexpr ObjectWithRefCount(Args&&... args) : ref_count(1), object(util::forward<Args>(args)...) {}

        // Shared storage internally stores a pointer directly to the underlying T object. To access the reference
        // count, shared storage looks directly in front of the object. To ensure this layout is chosen by the compiler,
        // we must pad this object if the underlying T is over-aligned.
        constexpr static usize alignment = container::max(alignof(T), alignof(sync::Atomic<usize>));
        constexpr static usize padding_size =
            alignment <= alignof(sync::Atomic<usize>) ? 0 : sizeof(alignment - sizeof(sync::Atomic<usize>));

        static ObjectWithRefCount* from_object_pointer(T* object) {
            auto* byte_pointer = reinterpret_cast<Byte*>(object);
            byte_pointer -= padding_size + sizeof(sync::Atomic<usize>);
            return reinterpret_cast<ObjectWithRefCount*>(byte_pointer);
        }

        T* to_object_pointer() { return util::addressof(object); }

        [[no_unique_address]] Array<Byte, padding_size> padding;
        sync::Atomic<usize> ref_count;
        T object;
    };
}

struct SharedStorage {
private:
    template<typename>
    friend struct detail::SharedStorageManage;

public:
    using Manage = meta::Type<detail::SharedStorageManage<SharedStorage>>;
    using Interface = meta::List<Manage>;

    constexpr static StorageCategory storage_category() { return StorageCategory::Copyable; }

    template<typename T>
    constexpr static bool creation_is_fallible(InPlaceType<T>) {
        return true;
    }

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...> && alignof(T) <= alignof(usize))
    constexpr static auto init(SharedStorage* self, InPlaceType<T>, Args&&... args) {
        using Store = detail::ObjectWithRefCount<T>;
        return platform::DefaultFallibleAllocator<Store>().allocate(1) % [&](container::Allocation<Store> result) {
            util::construct_at(result.data, util::forward<Args>(args)...);
            self->m_pointer = result.data->to_object_pointer();
        };
    }

    constexpr SharedStorage() {}

    SharedStorage(SharedStorage const&) = default;
    SharedStorage& operator=(SharedStorage const&) = default;

    ~SharedStorage() = default;

    constexpr static void copy_construct(concepts::VTableFor<Interface> auto const&, SharedStorage* dest,
                                         SharedStorage const* source) {
        if ((dest->m_pointer = source->m_pointer)) {
            dest->fetch_add_ref_count();
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
            if (self->fetch_sub_ref_count() == 1) {
                auto const fp = vtable[Manage {}];
                fp(self);
            }
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
    constexpr explicit SharedStorage(void* pointer) : m_pointer(pointer) {}

    usize fetch_sub_ref_count() {
        auto* ref_count = static_cast<sync::Atomic<usize>*>(m_pointer) - 1;
        return ref_count->fetch_sub(1, sync::MemoryOrder::Relaxed);
    }

    usize fetch_add_ref_count() {
        auto* ref_count = static_cast<sync::Atomic<usize>*>(m_pointer) - 1;
        return ref_count->fetch_add(1, sync::MemoryOrder::AcquireRelease);
    }

    void* m_pointer { nullptr };
};

namespace detail {
    template<typename SharedStorage>
    template<typename T>
    void SharedStorageManage<SharedStorage>::operator()(T& object) const {
        auto* pointer = detail::ObjectWithRefCount<T>::from_object_pointer(util::addressof(object));
        util::destroy_at(pointer);
        platform::DefaultFallibleAllocator<detail::ObjectWithRefCount<T>>().deallocate(pointer, 1);
    }
}
}
