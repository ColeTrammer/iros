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
    constexpr static auto bits_into_value(BitSet<bit_size> const& bit_set) -> bool {
        return bit_set[index];
    }

    constexpr BitFlag(bool value) : m_value(value) {}
    constexpr auto get() const -> bool { return m_value; }

private:
    bool m_value;
};
}

namespace di {
using bit::BitFlag;
}
