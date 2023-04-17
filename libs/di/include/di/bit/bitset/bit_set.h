#pragma once

#include <di/assert/assert_bool.h>
#include <di/bit/bitset/bit_proxy_reference.h>
#include <di/math/divide_round_up.h>
#include <di/types/prelude.h>
#include <di/vocab/array/prelude.h>

namespace di::bit {
template<size_t extent>
class [[gnu::packed]] BitSet {
public:
    constexpr BitSet() { m_storage.fill(0); }

    BitSet(BitSet const&) = default;
    BitSet& operator=(BitSet const&) = default;

    constexpr auto operator[](size_t index) {
        DI_ASSERT(index < extent);
        auto byte_index = index / 8u;
        auto bit_index = index % 8u;
        return detail::BitProxyReference(m_storage.data() + byte_index, bit_index);
    }
    constexpr bool operator[](size_t index) const {
        DI_ASSERT(index < extent);
        auto byte_index = index / 8u;
        auto bit_index = index % 8u;
        return detail::BitProxyReference(const_cast<u8*>(m_storage.data()) + byte_index, bit_index);
    }

    constexpr size_t size() const { return extent; }

private:
    Array<u8, math::divide_round_up(extent, 8u)> m_storage;
};
}
