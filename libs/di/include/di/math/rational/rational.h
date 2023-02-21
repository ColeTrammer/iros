#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/convertible_to.h>
#include <di/math/lcm.h>

namespace di::math {
template<concepts::SignedInteger T>
class Rational {
public:
    constexpr Rational() : m_numerator(1), m_denominator(1) {}

    constexpr explicit Rational(T numerator, T denominator = 1) : m_numerator(numerator), m_denominator(denominator) {
        // DI_ASSERT_NOT_EQ(m_denominator, T(0));

        normalize();
    }

    template<concepts::SignedInteger U>
    requires(concepts::ConstructibleFrom<T, U>)
    constexpr explicit(sizeof(U) <= sizeof(T)) operator Rational<U>() const {
        return Rational<U> { U(numerator()), U(denominator()) };
    }

    constexpr T numerator() const { return m_numerator; }
    constexpr T denominator() const { return m_denominator; }

    constexpr bool negative() const { return m_numerator < 0; }

    constexpr Rational add(Rational other) const {
        auto common_denominator = math::lcm(this->denominator(), other.denominator());

        auto a_scaled_numerator = this->numerator() * (common_denominator / this->denominator());
        auto b_scaled_numerator = other.numerator() * (common_denominator / other.denominator());

        return Rational { a_scaled_numerator + b_scaled_numerator, common_denominator };
    }

    constexpr Rational multiply(Rational other) const {
        return Rational { this->numerator() * other.numerator(), this->denominator() * other.denominator() };
    }

    constexpr Rational subtract(Rational other) const { return add(other.negated()); }
    constexpr Rational divide(Rational other) const { return multiply(other.inverted()); }

    constexpr Rational negated() const { return Rational { -numerator(), denominator() }; }
    constexpr Rational inverted() const { return Rational { denominator(), numerator() }; }

    constexpr Rational& operator++() { return *this = add({}); }
    constexpr Rational operator++(int) {
        auto save = *this;
        *this = add({});
        return save;
    }

    constexpr Rational& operator--() { return *this = subtract({}); }
    constexpr Rational operator--(int) {
        auto save = *this;
        *this = subtract({});
        return save;
    }

    constexpr Rational& operator+=(Rational a) { return *this = add(a); }
    constexpr Rational operator+(Rational a) const { return add(a); }

    constexpr Rational& operator-=(Rational a) { return *this = subtract(a); }
    constexpr Rational operator-(Rational a) const { return subtract(a); }

    constexpr Rational& operator*=(Rational a) { return *this = multiply(a); }
    constexpr Rational operator*(Rational a) const { return multiply(a); }

    constexpr Rational& operator/=(Rational a) { return *this = divide(a); }
    constexpr Rational operator/(Rational a) const { return divide(a); }

    constexpr Rational operator-() const { return negated(); }

    // These fields are public to enable this class to be used
    // as a template argument.
    T m_numerator;
    T m_denominator;

private:
    constexpr friend bool operator==(Rational, Rational) = default;

    constexpr friend strong_ordering operator<=>(Rational a, Rational b) {
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
