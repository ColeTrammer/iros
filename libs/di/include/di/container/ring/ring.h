#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/allocator/allocate_many.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_many.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/ring/mutable_ring_interface.h>
#include <di/container/types/prelude.h>
#include <di/platform/prelude.h>
#include <di/types/prelude.h>
#include <di/util/deduce_create.h>
#include <di/util/exchange.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
template<typename T, concepts::Allocator Alloc = platform::DefaultAllocator>
class Ring : public MutableRingInterface<Ring<T, Alloc>, T> {
public:
    using Value = T;
    using ConstValue = T const;

    constexpr Ring() = default;
    constexpr Ring(Ring const&) = delete;
    constexpr Ring(Ring&& other)
        : m_data(util::exchange(other.m_data, nullptr))
        , m_size(util::exchange(other.m_size, 0))
        , m_capacity(util::exchange(other.m_capacity, 0))
        , m_head(util::exchange(other.m_head, 0))
        , m_tail(util::exchange(other.m_tail, 0))
        , m_allocator(util::move(other.m_allocator)) {}

    constexpr ~Ring() {
        this->clear();
        if (m_data) {
            di::deallocate_many<T>(m_allocator, m_data, m_capacity);
        }
    }

    constexpr Ring& operator=(Ring const&) = delete;
    constexpr Ring& operator=(Ring&& other) {
        this->m_data = util::exchange(other.m_data, nullptr);
        this->m_size = util::exchange(other.m_size, 0);
        this->m_capacity = util::exchange(other.m_capacity, 0);
        this->m_head = util::exchange(other.m_head, 0);
        this->m_tail = util::exchange(other.m_tail, 0);
        this->m_allocator = util::move(other.m_allocator);
        return *this;
    }

    constexpr Span<Value> span() { return { m_data, m_size }; }
    constexpr Span<ConstValue> span() const { return { m_data, m_size }; }

    constexpr usize capacity() const { return m_capacity; }
    constexpr usize max_size() const { return static_cast<usize>(-1); }

    constexpr auto reserve_from_nothing(usize n) {
        DI_ASSERT(capacity() == 0u);

        return as_fallible(di::allocate_many<T>(m_allocator, n)) % [&](AllocationResult<T> result) {
            auto [data, new_capacity] = result;
            m_data = data;
            m_capacity = new_capacity;
        } | try_infallible;
    }
    constexpr void assume_size(usize size) { m_size = size; }

    constexpr usize head() const { return m_head; }
    constexpr usize tail() const { return m_tail; }

    constexpr void assume_head(usize head) { m_head = head; }
    constexpr void assume_tail(usize tail) { m_tail = tail; }

private:
    T* m_data { nullptr };
    usize m_size { 0 };
    usize m_capacity { 0 };
    usize m_head { 0 };
    usize m_tail { 0 };
    [[no_unique_address]] Alloc m_allocator {};
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
Ring<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<Ring>, Con&&);
}

namespace di {
using container::Ring;
}
