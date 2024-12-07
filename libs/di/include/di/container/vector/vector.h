#pragma once

#include "di/assert/assert_bool.h"
#include "di/container/allocator/allocate_many.h"
#include "di/container/allocator/allocation_result.h"
#include "di/container/allocator/allocator.h"
#include "di/container/allocator/deallocate_many.h"
#include "di/container/allocator/fallible_allocator.h"
#include "di/container/allocator/infallible_allocator.h"
#include "di/container/concepts/prelude.h"
#include "di/container/meta/prelude.h"
#include "di/container/types/prelude.h"
#include "di/container/vector/mutable_vector_interface.h"
#include "di/container/vector/vector_forward_declaration.h"
#include "di/platform/prelude.h"
#include "di/types/prelude.h"
#include "di/util/deduce_create.h"
#include "di/util/exchange.h"
#include "di/vocab/expected/prelude.h"
#include "di/vocab/span/prelude.h"

namespace di::container {
template<typename T, concepts::Allocator Alloc>
class Vector : public MutableVectorInterface<Vector<T, Alloc>, T> {
public:
    using Value = T;
    using ConstValue = T const;

    constexpr Vector() = default;
    constexpr Vector(Vector const&) = delete;
    constexpr Vector(Vector&& other)
        : m_data(util::exchange(other.m_data, nullptr))
        , m_size(util::exchange(other.m_size, 0))
        , m_capacity(util::exchange(other.m_capacity, 0))
        , m_allocator(util::move(other.m_allocator)) {}

    constexpr ~Vector() { deallocate(); }

    constexpr auto operator=(Vector const&) -> Vector& = delete;
    constexpr auto operator=(Vector&& other) -> Vector& {
        deallocate();
        this->m_data = util::exchange(other.m_data, nullptr);
        this->m_size = util::exchange(other.m_size, 0);
        this->m_capacity = util::exchange(other.m_capacity, 0);
        this->m_allocator = util::move(other.m_allocator);
        return *this;
    }

    constexpr auto span() -> Span<Value> { return { m_data, m_size }; }
    constexpr auto span() const -> Span<ConstValue> { return { m_data, m_size }; }

    constexpr auto capacity() const -> usize { return m_capacity; }
    constexpr auto max_size() const -> usize { return static_cast<usize>(-1); }

    constexpr auto reserve_from_nothing(usize n) -> meta::AllocatorResult<Alloc> {
        DI_ASSERT(capacity() == 0U);

        return as_fallible(di::allocate_many<T>(m_allocator, n)) % [&](AllocationResult<T> result) {
            auto [data, new_capacity] = result;
            m_data = data;
            m_capacity = new_capacity;
        } | try_infallible;
    }
    constexpr void assume_size(usize size) { m_size = size; }

private:
    constexpr void deallocate() {
        this->clear();
        if (m_data) {
            di::deallocate_many<T>(m_allocator, m_data, m_capacity);
        }
    }

    T* m_data { nullptr };
    usize m_size { 0 };
    usize m_capacity { 0 };
    [[no_unique_address]] Alloc m_allocator {};
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<Vector>, Con&&) -> Vector<T>;
}

namespace di {
using container::Vector;
}
