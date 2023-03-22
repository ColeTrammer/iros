#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/vector/mutable_vector_interface.h>
#include <di/math/smallest_unsigned_type.h>
#include <di/meta/size_constant.h>
#include <di/types/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
struct NoCapacityError {};

template<typename T, typename SizeConstant>
class StaticVector : public MutableVectorInterface<StaticVector<T, SizeConstant>, T> {
private:
    constexpr static size_t inline_capacity = SizeConstant {};

    using SizeType = math::SmallestUnsignedType<inline_capacity>;

public:
    using Value = T;
    using ConstValue = T const;

    constexpr StaticVector() {
        for (auto& x : m_data) {
            x = T();
        }
    }

    StaticVector(StaticVector const&) = default;
    StaticVector(StaticVector&&) = default;

    StaticVector& operator=(StaticVector const&) = default;
    StaticVector& operator=(StaticVector&&) = default;

    constexpr Span<T> span() { return { m_data, m_size }; }
    constexpr Span<T const> span() const { return { m_data, m_size }; }

    constexpr size_t capacity() const { return inline_capacity; }
    constexpr size_t max_size() const { return inline_capacity; }

    constexpr Expected<void, NoCapacityError> reserve_from_nothing(size_t n) {
        DI_ASSERT_EQ(this->size(), 0u);
        if (n > inline_capacity) {
            return Unexpected(NoCapacityError {});
        }
        return {};
    }
    constexpr void assume_size(size_t size) { m_size = static_cast<SizeType>(size); }

private:
    T m_data[inline_capacity];
    SizeType m_size { 0 };
};
}
