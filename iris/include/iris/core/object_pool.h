#pragma once

#include <di/assert/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/allocator/allocate_many.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_many.h>
#include <di/container/intrusive/prelude.h>
#include <di/platform/prelude.h>
#include <di/util/prelude.h>
#include <iris/core/error.h>

namespace iris {
namespace detail {
    struct InternalObjectTag : di::IntrusiveForwardListTag<InternalObjectTag> {
        template<typename U>
        constexpr static bool is_sized(di::InPlaceType<U>) {
            return true;
        }
    };

    struct InternalObjectFreed : di::IntrusiveForwardListNode<InternalObjectTag> {};

    template<typename T>
    union InternalObject {
        InternalObject() {}

        T object;
        InternalObjectFreed freed;
    };
}

/// @brief A fixed-capacity object pool.
///
/// @tparam T The type of the objects to be stored in the pool. T must be derived from `di::IntrusiveListNode<>` and be
/// default constructible.
/// @tparam Alloc The allocator to be used to allocate the pool's storage.
///
/// @warning This class is not thread-safe. Use di::Synchronized<> to make it thread-safe.
template<typename T, di::concepts::FallibleAllocator Alloc = di::DefaultAllocator>
class ObjectPool {
public:
    static Expected<ObjectPool> create(usize requested_capacity) {
        auto pool = ObjectPool {};

        auto [storage, effective_capacity] =
            TRY(di::allocate_many<detail::InternalObject<T>>(pool.m_allocator, requested_capacity));
        pool.m_storage = storage;
        pool.m_capacity = effective_capacity;

        di::uninitialized_default_construct(pool.m_storage, pool.m_storage + pool.m_capacity);

        di::for_each(pool.m_storage, pool.m_storage + pool.m_capacity, [&](detail::InternalObject<T>& object) {
            di::construct_at(&object.freed);
            pool.m_free_list.push_front(object.freed);
        });

        return pool;
    }

    ObjectPool() = default;

    ObjectPool(ObjectPool&& other)
        : m_free_list(di::move(other.m_free_list))
        , m_storage(di::exchange(other.m_storage, nullptr))
        , m_capacity(di::exchange(other.m_capacity, 0))
        , m_allocator(di::move(other.m_allocator)) {
        ASSERT_EQ(m_capacity, m_free_list.size());
    }

    ~ObjectPool() { clear(); }

    ObjectPool& operator=(ObjectPool&& other) {
        clear();

        m_free_list = di::move(other.m_free_list);
        m_storage = di::exchange(other.m_storage, nullptr);
        m_capacity = di::exchange(other.m_capacity, 0);
        m_allocator = di::move(other.m_allocator);

        return *this;
    }

    void clear() {
        ASSERT_EQ(m_free_list.size(), m_capacity);

        if (m_capacity) {
            // NOTE: all objects must have been freed, so there is no need to call destructors.
            di::deallocate_many<detail::InternalObject<T>>(m_allocator, m_storage, m_capacity);
        }
    }

    Expected<T&> allocate() {
        if (m_free_list.empty()) {
            return di::Unexpected(Error::NotEnoughMemory);
        }
        auto* freed_pointer = &*m_free_list.pop_front();

        // NOTE: the freed object should have the same memory location as the internal object, so this *should* be safe.
        auto* internal_object = reinterpret_cast<detail::InternalObject<T>*>(freed_pointer);
        di::destroy_at(&internal_object->freed);
        di::construct_at(&internal_object->object);
        return internal_object->object;
    }

    void deallocate(T& object) {
        // NOTE: the object should have the same memory location as the internal object, so this *should* be safe.
        auto* internal_object = reinterpret_cast<detail::InternalObject<T>*>(&object);
        di::destroy_at(&internal_object->object);

        di::construct_at(&internal_object->freed);
        m_free_list.push_front(internal_object->freed);
    }

private:
    di::IntrusiveForwardList<detail::InternalObjectFreed, detail::InternalObjectTag> m_free_list;
    detail::InternalObject<T>* m_storage { nullptr };
    usize m_capacity { 0 };
    [[no_unique_address]] Alloc m_allocator {};
};
}
