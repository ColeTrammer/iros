#pragma once

#include <di/meta/language.h>
#include <di/types/prelude.h>

namespace di::random {
template<concepts::UnsignedInteger T, T a, T c, T m>
class LinearCongruentialEngine {
public:
    using Result = T;

    constexpr static auto multiplier = a;
    constexpr static auto increment = c;
    constexpr static auto modulus = m;
    constexpr static T default_seed = 1U;

    constexpr static auto min() -> T { return increment == 0U ? 1U : 0U; }
    constexpr static auto max() -> T { return modulus - 1U; }

private:
    constexpr auto safe_modulo(T value) -> T {
        if constexpr (modulus == 0U) {
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
            m_state = 1U;
        } else {
            m_state = safe_modulo(seed);
        }
    }

    constexpr auto operator()() -> T {
        if constexpr (multiplier == 0) {
            return safe_modulo(increment);
        } else {
            return m_state = safe_modulo(multiplier * m_state + increment);
        }
    }

    constexpr void discard(umax z) {
        for (; z != 0U; z--) {
            (*this)();
        }
    }

private:
    constexpr friend auto operator==(LinearCongruentialEngine const& x, LinearCongruentialEngine const& y) -> bool {
        return x.m_state == y.m_state;
    }

    T m_state;
};

using MinstdRand0 = LinearCongruentialEngine<u32, 16807, 0, 2147483647>;
using MinstdRand = LinearCongruentialEngine<u32, 48271, 0, 2147483647>;
}

namespace di {
using random::LinearCongruentialEngine;
using random::MinstdRand;
using random::MinstdRand0;
}
