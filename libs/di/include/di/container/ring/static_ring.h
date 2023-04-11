#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/ring/mutable_ring_interface.h>
#include <di/container/types/prelude.h>
#include <di/container/vector/static_vector.h>
#include <di/math/smallest_unsigned_type.h>
#include <di/meta/size_constant.h>
#include <di/types/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
template<typename T, typename SizeConstant>
class StaticRing : public MutableRingInterface<StaticRing<T, SizeConstant>, T> {
private:
    constexpr static usize inline_capacity = SizeConstant {};

    using SizeType = math::SmallestUnsignedType<inline_capacity>;

public:
    using Value = T;
    using ConstValue = T const;

    constexpr StaticRing() {
        for (auto& x : m_data) {
            x = T();
        }
    }

    StaticRing(StaticRing const&) = default;
    StaticRing(StaticRing&&) = default;

    StaticRing& operator=(StaticRing const&) = default;
    StaticRing& operator=(StaticRing&&) = default;

    constexpr Span<T> span() { return { m_data, m_size }; }
    constexpr Span<T const> span() const { return { m_data, m_size }; }

    constexpr usize capacity() const { return inline_capacity; }
    constexpr usize max_size() const { return inline_capacity; }

    constexpr Expected<void, NoCapacityError> reserve_from_nothing(usize n) {
        DI_ASSERT_EQ(this->size(), 0u);
        if (n > inline_capacity) {
            return Unexpected(NoCapacityError {});
        }
        return {};
    }
    constexpr void assume_size(usize size) { m_size = static_cast<SizeType>(size); }

    constexpr usize head() const { return m_head; }
    constexpr usize tail() const { return m_tail; }

    constexpr void assume_head(usize head) { m_head = static_cast<SizeType>(head); }
    constexpr void assume_tail(usize tail) { m_tail = static_cast<SizeType>(tail); }

private:
    T m_data[inline_capacity];
    SizeType m_size { 0 };
    SizeType m_head { 0 };
    SizeType m_tail { 0 };
};
}
