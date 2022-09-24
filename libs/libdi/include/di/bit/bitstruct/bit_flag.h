#pragma once

#include <di/bit/bitstruct/bit_tag.h>

namespace di::bit {
template<size_t index>
struct BitFlag {
    using Value = bool;

    template<size_t bit_size>
    constexpr static void value_into_bits(BitSet<bit_size>& bit_set, bool value) {
        bit_set[index] = value;
    }
    template<size_t bit_size>
    constexpr static bool bits_into_value(BitSet<bit_size> const& bit_set) {
        return bit_set[index];
    }

    constexpr BitFlag(bool value) : m_value(value) {}
    constexpr bool get() const { return m_value; }

private:
    bool m_value;
};
}