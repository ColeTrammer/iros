#pragma once

#include <di/bit/bitstruct/bit_tag.h>
#include <di/math/smallest_unsigned_type.h>

namespace di::bit {
template<size_t index, size_t bit_count>
requires(bit_count <= 63)
struct BitField {
    using Value = math::SmallestUnsignedType<(1ULL << bit_count) - 1>;

    template<size_t bit_size>
    constexpr static void value_into_bits(BitSet<bit_size>& bit_set, Value value) {
        for (size_t i = 0; i < bit_count; i++) {
            bit_set[index + i] = !!(value & (1ULL << i));
        }
    }

    template<size_t bit_size>
    constexpr static auto bits_into_value(BitSet<bit_size> const& bit_set) -> Value {
        auto result = Value { 0 };
        for (size_t i = 0; i < bit_count; i++) {
            result |= (bit_set[index + i] << i);
        }
        return result;
    }

    constexpr BitField(Value value) : m_value(value) {}
    constexpr auto get() const -> Value { return m_value; }

private:
    Value m_value;
};
}

namespace di {
using bit::BitField;
}
