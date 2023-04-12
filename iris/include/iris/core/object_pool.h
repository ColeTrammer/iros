#pragma once

#include <di/prelude.h>

#include <iris/core/error.h>

namespace iris {
/// @brief A fixed-capacity object pool.
///
/// @tparam T The type of the objects to be stored in the pool. T must be derived from `di::IntrusiveListNode<>` and be
/// default constructible.
/// @tparam Alloc The allocator to be used to allocate the pool's storage.
///
/// @warning This class is not thread-safe. Use di::Synchronized<> to make it thread-safe.
template<di::concepts::DerivedFrom<di::IntrusiveListNode<>> T,
         di::concepts::AllocatorOf<T> Alloc = di::DefaultAllocator<T>>
requires(di::concepts::DefaultConstructible<T>)
class ObjectPool {
public:
    static Expected<ObjectPool> create(usize requested_capacity) {
        auto pool = ObjectPool {};

        auto [storage, effective_capacity] = TRY(Alloc().allocate(requested_capacity));
        pool.m_storage = storage;
        pool.m_capacity = effective_capacity;

        di::container::uninitialized_default_construct(pool.m_storage, pool.m_storage + pool.m_capacity);

        di::for_each(pool.m_storage, pool.m_storage + pool.m_capacity, [&](T& object) {
            pool.m_free_list.push_back(object);
        });

        pool.m_free_list_size = pool.m_capacity;
        return pool;
    }

    ObjectPool() = default;

    ObjectPool(ObjectPool&& other)
        : m_free_list(di::move(other.m_free_list))
        , m_storage(di::exchange(other.m_storage, nullptr))
        , m_capacity(di::exchange(other.m_capacity, 0))
        , m_free_list_size(di::exchange(other.m_free_list_size, 0)) {
        ASSERT_EQ(m_capacity, m_free_list_size);
    }

    ~ObjectPool() { clear(); }

    ObjectPool& operator=(ObjectPool&& other) {
        clear();

        m_free_list = di::move(other.m_free_list);
        m_storage = di::exchange(other.m_storage, nullptr);
        m_capacity = di::exchange(other.m_capacity, 0);
        m_free_list_size = di::exchange(other.m_free_list_size, 0);

        return *this;
    }

    void clear() {
        ASSERT_EQ(m_free_list_size, m_capacity);

        if (m_capacity) {
            di::destroy(m_storage, m_storage + m_capacity);

            Alloc().deallocate(m_storage, m_capacity);
        }
    }

    Expected<T&> allocate() {
        if (m_free_list.empty()) {
            return di::Unexpected(Error::NotEnoughMemory);
        }
        m_free_list_size--;
        return *m_free_list.pop_back();
    }

    void deallocate(T& object) {
        m_free_list.push_back(object);
        m_free_list_size++;
    }

private:
    di::IntrusiveList<T> m_free_list;
    T* m_storage { nullptr };
    usize m_capacity { 0 };
    usize m_free_list_size { 0 };
};
}
