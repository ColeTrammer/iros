#pragma once

#include <di/math/bigint/fixed_ops.h>
#include <di/types/prelude.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::math {
/// @brief A fixed-width unsigned integer.
///
/// @tparam bits The number of bits in the integer.
///
/// @note This can be used fallback implementation for when the compiler doesn't support 128-bit integers. As such, its
/// internal layout must by ABI compatible with the compiler's 128-bit integer type.
template<usize bits>
requires(bits % sizeof(bigint::StorageType) == 0)
class FixedUnsigned {
private:
    constexpr static usize word_count = bits / sizeof(bigint::StorageType) / 8;

    using Ops = bigint::FixedOps<word_count>;

public:
    FixedUnsigned() = default;

    constexpr FixedUnsigned(bigint::StorageType value) { m_storage[0] = value; }

    constexpr FixedUnsigned operator/(FixedUnsigned const& divisor) const {
        auto division_result = FixedUnsigned();
        auto modulo_result = FixedUnsigned();
        Ops::div_mod(this->span(), divisor.span(), division_result.span(), modulo_result.span());
        return division_result;
    }

    constexpr FixedUnsigned operator%(FixedUnsigned const& divisor) const {
        auto division_result = FixedUnsigned();
        auto modulo_result = FixedUnsigned();
        Ops::div_mod(this->span(), divisor.span(), division_result.span(), modulo_result.span());
        return modulo_result;
    }

private:
    constexpr friend bool operator==(FixedUnsigned const& a, FixedUnsigned const& b) {
        return Ops::compare(a.span(), b.span()) == 0;
    }

    constexpr friend auto operator<=>(FixedUnsigned const& a, FixedUnsigned const& b) {
        return Ops::compare(a.span(), b.span());
    }

    constexpr auto span() { return m_storage.span(); }
    constexpr auto span() const { return m_storage.span(); }

    di::Array<bigint::StorageType, word_count> m_storage {};
};

using u128_fallback = FixedUnsigned<128>;
using u256 = FixedUnsigned<256>;
}

namespace di {
using math::FixedUnsigned;
using math::u128_fallback;
using math::u256;
}
