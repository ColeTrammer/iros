#pragma once

#include <di/container/allocator/allocator.h>
#include <di/container/allocator/allocator_of.h>
#include <di/container/vector/mutable_vector_interface.h>
#include <di/types/prelude.h>
#include <di/util/exchange.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
template<typename T, concepts::AllocatorOf<T> Alloc = Allocator<T>>
class Vector : public MutableVectorInterface<Vector<T>, T> {
public:
    using Value = T;
    using ConstValue = T const;

    constexpr Vector() = default;
    constexpr Vector(Vector const&) = delete;
    constexpr Vector(Vector&& other)
        : m_data(util::exchange(other.m_data, nullptr))
        , m_size(util::exchange(other.m_size, 0))
        , m_capacity(util::exchange(other.m_capacity, 0)) {}

    constexpr ~Vector() {
        this->clear();
        if (m_data) {
            Alloc().deallocate(m_data, m_capacity);
        }
    }

    constexpr Vector& operator=(Vector const&) = delete;
    constexpr Vector& operator=(Vector&& other) {
        this->m_data = util::exchange(other.m_data, nullptr);
        this->m_size = util::exchange(other.m_size, 0);
        this->m_capacity = util::exchange(other.m_capacity, 0);
        return *this;
    }

    constexpr Span<Value> span() { return { m_data, m_size }; }
    constexpr Span<ConstValue> span() const { return { m_data, m_size }; }

    constexpr size_t capacity() const { return m_capacity; }
    constexpr size_t max_size() const { return static_cast<size_t>(-1); }

    constexpr void reserve_from_nothing(size_t n) {
        // DI_ASSERT( m_capacity == 0 )
        auto [data, new_capacity] = Alloc().allocate(n);
        m_data = data;
        m_capacity = new_capacity;
    }
    constexpr void assume_size(size_t size) { m_size = size; }

private:
    T* m_data { nullptr };
    size_t m_size { 0 };
    size_t m_capacity { 0 };
};
}
