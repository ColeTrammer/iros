#pragma once

#include <di/math/numeric_limits.h>
#include <di/meta/language.h>
#include <di/vocab/optional/lift_bool.h>
#include <di/vocab/optional/prelude.h>

namespace di::math {
template<concepts::Integer T>
class Checked {
public:
    Checked() = default;

    constexpr Checked(T value) : m_value(value) {}

    constexpr Optional<T> value() const { return m_invalid ? nullopt : Optional(m_value); }

    constexpr bool valid() const { return !m_invalid; }
    constexpr bool invalid() const { return m_invalid; }

    constexpr Checked& operator+=(Checked value) {
        m_invalid |= value.m_invalid;
        m_invalid |= __builtin_add_overflow(m_value, value.m_value, &m_value);
        return *this;
    }

    constexpr Checked& operator-=(Checked value) {
        m_invalid |= value.m_invalid;
        m_invalid |= __builtin_sub_overflow(m_value, value.m_value, &m_value);
        return *this;
    }

    constexpr Checked& operator*=(Checked value) {
        m_invalid |= value.m_invalid;
        m_invalid |= __builtin_mul_overflow(m_value, value.m_value, &m_value);
        return *this;
    }

    constexpr Checked& operator/=(Checked value) {
        m_invalid |= value.m_invalid;

        // Division is undefined for signed integers if the divisor is -1 and the dividend is the minimum value. Also,
        // division by zero is undefined.
        if constexpr (concepts::Signed<T>) {
            m_invalid |= value.m_value == -1 && m_value == NumericLimits<T>::min;
        }
        m_invalid |= value.m_value == 0;
        if (!m_invalid) {
            m_value /= value.m_value;
        }

        return *this;
    }

    constexpr Checked& operator%=(Checked value) {
        m_invalid |= value.m_invalid;

        // Division is undefined for signed integers if the divisor is -1 and the dividend is the minimum value. Also,
        // division by zero is undefined.
        if constexpr (concepts::Signed<T>) {
            m_invalid |= value.m_value == -1 && m_value == NumericLimits<T>::min;
        }
        m_invalid |= value.m_value == 0;
        if (!m_invalid) {
            m_value %= value.m_value;
        }

        return *this;
    }

    constexpr Checked& operator&=(Checked value) {
        m_invalid |= value.m_invalid;
        m_value &= value.m_value;
        return *this;
    }

    constexpr Checked& operator|=(Checked value) {
        m_invalid |= value.m_invalid;
        m_value |= value.m_value;
        return *this;
    }

    constexpr Checked& operator^=(Checked value) {
        m_invalid |= value.m_invalid;
        m_value ^= value.m_value;
        return *this;
    }

    constexpr Checked& operator<<=(Checked value) {
        m_invalid |= value.m_invalid;

        // Shifting by a negative amount is undefined, as it shifting by more than the number of bits in the "promoted"
        // type. This implementation is more conservative, by ignoring promotions.
        m_invalid |= value.m_value < 0 || value.m_value >= NumericLimits<meta::MakeUnsigned<T>>::digits;
        if (!m_invalid) {
            m_value <<= value.m_value;
        }

        return *this;
    }

    constexpr Checked& operator>>=(Checked value) {
        m_invalid |= value.m_invalid;

        // Shifting by a negative amount is undefined, as it shifting by more than the number of bits in the "promoted"
        // type. This implementation is more conservative, by ignoring promotions.
        m_invalid |= value.m_value < 0 || value.m_value >= NumericLimits<meta::MakeUnsigned<T>>::digits;
        if (!m_invalid) {
            m_value >>= value.m_value;
        }

        return *this;
    }

    constexpr Checked& operator++() {
        *this += 1;
        return *this;
    }

    constexpr Checked& operator--() {
        *this -= 1;
        return *this;
    }

    constexpr Checked operator++(int) {
        auto result = *this;
        ++*this;
        return result;
    }

    constexpr Checked operator--(int) {
        auto result = *this;
        --*this;
        return result;
    }

    constexpr Checked operator+() const { return *this; }
    constexpr Checked operator-() const {
        // Negating the minimum value is undefined for signed integers.
        if constexpr (concepts::Signed<T>) {
            if (m_value == NumericLimits<T>::min) {
                auto result = *this;
                result.m_invalid = true;
                return result;
            }
        }
        return Checked(-m_value);
    }

    constexpr Checked operator~() const { return Checked(~m_value); }

    constexpr Checked operator&(Checked value) const {
        auto result = *this;
        result &= value;
        return result;
    }

    constexpr Checked operator|(Checked value) const {
        auto result = *this;
        result |= value;
        return result;
    }

    constexpr Checked operator^(Checked value) const {
        auto result = *this;
        result ^= value;
        return result;
    }

    constexpr Checked operator<<(Checked value) const {
        auto result = *this;
        result <<= value;
        return result;
    }

    constexpr Checked operator>>(Checked value) const {
        auto result = *this;
        result >>= value;
        return result;
    }

    constexpr Checked operator+(Checked value) const {
        auto result = *this;
        result += value;
        return result;
    }

    constexpr Checked operator-(Checked value) const {
        auto result = *this;
        result -= value;
        return result;
    }

    constexpr Checked operator*(Checked value) const {
        auto result = *this;
        result *= value;
        return result;
    }

    constexpr Checked operator/(Checked value) const {
        auto result = *this;
        result /= value;
        return result;
    }

    constexpr Checked operator%(Checked value) const {
        auto result = *this;
        result %= value;
        return result;
    }

private:
    T m_value {};
    bool m_invalid { false };
};
}

namespace di {
using math::Checked;
}
