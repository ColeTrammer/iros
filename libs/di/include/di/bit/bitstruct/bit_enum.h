#pragma once

#include "di/bit/bitstruct/bit_field.h"
#include "di/util/to_underlying.h"

namespace di::bit {
template<typename T, size_t index, size_t bit_count>
requires(bit_count <= 63)
struct BitEnum {
    using Value = T;

    template<size_t bit_size>
    constexpr static void value_into_bits(BitSet<bit_size>& bit_set, Value value) {
        BitField<index, bit_count>::value_into_bits(bit_set, util::to_underlying(value));
    }

    template<size_t bit_size>
    constexpr static auto bits_into_value(BitSet<bit_size> const& bit_set) -> T {
        return static_cast<T>(BitField<index, bit_count>::bits_into_value(bit_set));
    }

    constexpr BitEnum(T value) : m_value(value) {}
    constexpr auto get() const -> T { return m_value; }

private:
    T m_value;
};
}

namespace di {
using bit::BitEnum;
}
