#pragma once

#include "di/assert/assert_bool.h"
#include "di/math/lcm.h"
#include "di/meta/operations.h"

namespace di::math {
template<concepts::SignedInteger T>
class Rational {
public:
    constexpr Rational() : m_numerator(1), m_denominator(1) {}

    constexpr explicit Rational(T numerator, T denominator = 1) : m_numerator(numerator), m_denominator(denominator) {
        DI_ASSERT(m_denominator != T(0));

        normalize();
    }

    template<concepts::SignedInteger U>
    requires(concepts::ConstructibleFrom<T, U>)
    constexpr explicit(sizeof(U) <= sizeof(T)) operator Rational<U>() const {
        return Rational<U> { U(numerator()), U(denominator()) };
    }

    constexpr auto numerator() const -> T { return m_numerator; }
    constexpr auto denominator() const -> T { return m_denominator; }

    constexpr auto negative() const -> bool { return m_numerator < 0; }

    constexpr auto add(Rational other) const -> Rational {
        auto common_denominator = math::lcm(this->denominator(), other.denominator());

        auto a_scaled_numerator = this->numerator() * (common_denominator / this->denominator());
        auto b_scaled_numerator = other.numerator() * (common_denominator / other.denominator());

        return Rational { a_scaled_numerator + b_scaled_numerator, common_denominator };
    }

    constexpr auto multiply(Rational other) const -> Rational {
        return Rational { this->numerator() * other.numerator(), this->denominator() * other.denominator() };
    }

    constexpr auto subtract(Rational other) const -> Rational { return add(other.negated()); }
    constexpr auto divide(Rational other) const -> Rational { return multiply(other.inverted()); }

    constexpr auto negated() const -> Rational { return Rational { -numerator(), denominator() }; }
    constexpr auto inverted() const -> Rational { return Rational { denominator(), numerator() }; }

    constexpr auto operator++() -> Rational& { return *this = add({}); }
    constexpr auto operator++(int) -> Rational {
        auto save = *this;
        *this = add({});
        return save;
    }

    constexpr auto operator--() -> Rational& { return *this = subtract({}); }
    constexpr auto operator--(int) -> Rational {
        auto save = *this;
        *this = subtract({});
        return save;
    }

    constexpr auto operator+=(Rational a) -> Rational& { return *this = add(a); }
    constexpr auto operator+(Rational a) const -> Rational { return add(a); }

    constexpr auto operator-=(Rational a) -> Rational& { return *this = subtract(a); }
    constexpr auto operator-(Rational a) const -> Rational { return subtract(a); }

    constexpr auto operator*=(Rational a) -> Rational& { return *this = multiply(a); }
    constexpr auto operator*(Rational a) const -> Rational { return multiply(a); }

    constexpr auto operator/=(Rational a) -> Rational& { return *this = divide(a); }
    constexpr auto operator/(Rational a) const -> Rational { return divide(a); }

    constexpr auto operator-() const -> Rational { return negated(); }

    // These fields are public to enable this class to be used
    // as a template argument.
    T m_numerator;
    T m_denominator;

private:
    constexpr friend auto operator==(Rational, Rational) -> bool = default;

    constexpr friend auto operator<=>(Rational a, Rational b) -> strong_ordering {
        if (auto result = b.negative() <=> a.negative(); result != 0) {
            return result;
        }

        auto difference = a.subtract(b);
        return difference.numerator() <=> T(0);
    }

    constexpr void normalize() {
        if ((numerator() < T(0) && denominator() < T(0)) || (numerator() >= T(0) && denominator() < T(0))) {
            m_numerator = -m_numerator;
            m_denominator = -m_denominator;
        }

        if (numerator() == T(0)) {
            m_denominator = 1;
            return;
        }

        auto gcd = math::gcd(numerator(), denominator());
        m_numerator /= gcd;
        m_denominator /= gcd;
    }
};

template<typename T, typename U>
Rational(T, U) -> Rational<meta::CommonType<T, U>>;
}

namespace di {
using math::Rational;
}
