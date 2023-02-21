#pragma once

#include <di/concepts/unsigned_integer.h>
#include <di/types/prelude.h>

namespace di::random {
template<concepts::UnsignedInteger T, T a, T c, T m>
class LinearCongruentialEngine {
public:
    using Result = T;

    constexpr static auto multiplier = a;
    constexpr static auto increment = c;
    constexpr static auto modulus = m;
    constexpr static T default_seed = 1u;

    constexpr static T min() { return increment == 0u ? 1u : 0u; }
    constexpr static T max() { return modulus - 1u; }

private:
    constexpr T safe_modulo(T value) {
        if constexpr (modulus == 0u) {
            return value;
        } else {
            return value % modulus;
        }
    }

public:
    constexpr LinearCongruentialEngine() : LinearCongruentialEngine(default_seed) {}

    constexpr explicit LinearCongruentialEngine(T seed) { this->seed(seed); }

    constexpr void seed(T seed = default_seed) {
        if (safe_modulo(increment) == 0 && safe_modulo(seed) == 0) {
            m_state = 1u;
        } else {
            m_state = safe_modulo(seed);
        }
    }

    constexpr T operator()() {
        if constexpr (multiplier == 0) {
            return safe_modulo(increment);
        } else {
            return m_state = safe_modulo(multiplier * m_state + increment);
        }
    }

    constexpr void discard(umax z) {
        for (; z != 0u; z--) {
            (*this)();
        }
    }

private:
    constexpr friend bool operator==(LinearCongruentialEngine const& x, LinearCongruentialEngine const& y) {
        return x.m_state == y.m_state;
    }

    T m_state;
};

using MinstdRand0 = LinearCongruentialEngine<u32, 16807, 0, 2147483647>;
using MinstdRand = LinearCongruentialEngine<u32, 48271, 0, 2147483647>;
}
