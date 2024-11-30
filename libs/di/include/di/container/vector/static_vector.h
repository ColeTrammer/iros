#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/vector/mutable_vector_interface.h>
#include <di/math/smallest_unsigned_type.h>
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

    auto operator=(StaticVector const&) -> StaticVector& = default;
    auto operator=(StaticVector&&) -> StaticVector& = default;

    constexpr auto span() -> Span<T> { return { m_data, m_size }; }
    constexpr auto span() const -> Span<T const> { return { m_data, m_size }; }

    constexpr auto capacity() const -> size_t { return inline_capacity; }
    constexpr auto max_size() const -> size_t { return inline_capacity; }

    constexpr auto reserve_from_nothing(size_t n) -> Expected<void, NoCapacityError> {
        DI_ASSERT(this->size() == 0U);
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

namespace di {
using container::StaticVector;
}
