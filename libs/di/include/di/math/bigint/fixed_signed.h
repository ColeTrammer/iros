#pragma once

#include <di/math/bigint/fixed_ops.h>
#include <di/types/prelude.h>
#include <di/vocab/array/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::math {
/// @brief A fixed-width signed integer.
///
/// @tparam bits The number of bits in the integer.
///
/// @note This can be used fallback implementation for when the compiler doesn't support 128-bit integers. As such, its
/// internal layout must by ABI compatible with the compiler's 128-bit integer type.
template<usize bits>
requires(bits % sizeof(bigint::StorageType) == 0)
class FixedSigned {
private:
    constexpr static usize word_count = bits / sizeof(bigint::StorageType) / 8;

    using Ops = bigint::FixedOps<word_count>;

public:
    FixedSigned() = default;

    constexpr FixedSigned(bigint::StorageType value) { m_storage[0] = value; }

    constexpr FixedSigned operator/(FixedSigned const& divisor) const {
        auto division_result = FixedSigned();
        auto modulo_result = FixedSigned();
        auto dividend_negative = Ops::twos_complement_negative(this->span());
        auto divisor_negative = Ops::twos_complement_negative(divisor.span());
        switch (dividend_negative + 2 * divisor_negative) {
            // + / + -> -
            case 0: {
                Ops::div_mod(this->span(), divisor.span(), division_result.span(), modulo_result.span());
                break;
            }
            // - / + -> -
            case 1: {
                auto dividend = *this;
                Ops::negate(dividend.span());
                Ops::div_mod(dividend.span(), divisor.span(), division_result.span(), modulo_result.span());
                Ops::negate(division_result.span());
                break;
            }
            // + / - -> -
            case 2: {
                auto divisor_copy = divisor;
                Ops::negate(divisor_copy.span());
                Ops::div_mod(this->span(), divisor_copy.span(), division_result.span(), modulo_result.span());
                Ops::negate(division_result.span());
                break;
            }
            // - / - -> +
            case 3: {
                auto dividend = *this;
                auto divisor_copy = divisor;
                Ops::negate(dividend.span());
                Ops::negate(divisor_copy.span());
                Ops::div_mod(dividend.span(), divisor_copy.span(), division_result.span(), modulo_result.span());
                break;
            }
        }
        return division_result;
    }

    constexpr FixedSigned operator%(FixedSigned const& divisor) const {
        auto division_result = FixedSigned();
        auto modulo_result = FixedSigned();
        auto dividend_negative = Ops::twos_complement_negative(this->span());
        auto divisor_negative = Ops::twos_complement_negative(divisor.span());
        switch (dividend_negative + 2 * divisor_negative) {
            // + % + -> -
            case 0: {
                Ops::div_mod(this->span(), divisor.span(), division_result.span(), modulo_result.span());
                break;
            }
            // - % + -> -
            case 1: {
                auto dividend = *this;
                Ops::negate(dividend.span());
                Ops::div_mod(dividend.span(), divisor.span(), division_result.span(), modulo_result.span());
                Ops::negate(modulo_result.span());
                break;
            }
            // + % - -> +
            case 2: {
                auto divisor_copy = divisor;
                Ops::negate(divisor_copy.span());
                Ops::div_mod(this->span(), divisor_copy.span(), division_result.span(), modulo_result.span());
                break;
            }
            // - % - -> -
            case 3: {
                auto dividend = *this;
                auto divisor_copy = divisor;
                Ops::negate(dividend.span());
                Ops::negate(divisor_copy.span());
                Ops::div_mod(dividend.span(), divisor_copy.span(), division_result.span(), modulo_result.span());
                Ops::negate(modulo_result.span());
                break;
            }
        }
        return modulo_result;
    }

    constexpr FixedSigned operator-() const {
        auto result = *this;
        Ops::negate(result.span());
        return result;
    }

private:
    constexpr friend bool operator==(FixedSigned const& a, FixedSigned const& b) {
        return Ops::compare(a.span(), b.span()) == 0;
    }

    constexpr friend auto operator<=>(FixedSigned const& a, FixedSigned const& b) {
        if (auto result = Ops::twos_complement_negative(a.span()) <=> Ops::twos_complement_negative(b.span());
            result != 0) {
            return result;
        }
        return Ops::compare(a.span(), b.span());
    }

    constexpr auto span() { return m_storage.span(); }
    constexpr auto span() const { return m_storage.span(); }

    di::Array<bigint::StorageType, word_count> m_storage {};
};

using i128_fallback = FixedSigned<128>;
using i256 = FixedSigned<256>;
}
