#pragma once

#include <di/bit/endian/endian.h>
#include <di/math/abs.h>
#include <di/math/constants.h>
#include <di/meta/language.h>
#include <di/types/byte.h>
#include <di/types/floats.h>
#include <di/util/bit_cast.h>
#include <di/vocab/array/array.h>

namespace di::math::detail {
struct Signbit {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x) -> T {
        auto as_bytes = di::bit_cast<di::Array<byte, sizeof(T)>>(x);
        auto msb = [&] {
            if constexpr (Endian::Little == Endian::Native) {
                return as_bytes[sizeof(T) - 1];
            } else {
                return as_bytes[0];
            }
        }();
        return (msb & byte(0b1000'0000)) != byte(0);
    }

    template<concepts::Integral T>
    constexpr static auto operator()(T x) -> bool {
        return Signbit::operator()(f64(x));
    }
};
}

namespace di {
constexpr inline auto signbit = math::detail::Signbit {};
}

namespace di::math::detail {
struct Copysign {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x, T y) -> T {
        auto sign = signbit(y);

        auto as_bytes = di::bit_cast<di::Array<byte, sizeof(T)>>(x);
        auto& msb = [&] -> byte& {
            if constexpr (Endian::Little == Endian::Native) {
                return as_bytes[sizeof(T) - 1];
            } else {
                return as_bytes[0];
            }
        }();

        msb &= ~(byte(1) << 7);
        msb |= (byte(sign) << 7);

        return di::bit_cast<T>(as_bytes);
    }

    template<concepts::Integral T>
    constexpr static auto operator()(T x, T y) -> f64 {
        return Copysign::operator()(f64(x), f64(y));
    }
};
}

namespace di {
constexpr inline auto copysign = math::detail::Copysign {};
}

namespace di::math::detail {
struct Round {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x) -> T {
        // FIXME: this is not sufficently precise.
        if (x < 0) {
            return -Round::operator()(-x);
        }
        // NOLINTNEXTLINE(bugprone-incorrect-roundings)
        return T(i64(x + 0.5));
    }

    template<concepts::Integral T>
    constexpr static auto operator()(T x) -> f64 {
        return Round::operator()(f64(x));
    }
};
}

namespace di {
constexpr inline auto round = math::detail::Round {};
}

namespace di::math::detail {
struct Remainder {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x, T y) -> T {
        // NOTE: this probably isn't a sufficently precise implemenation.
        auto quotient = round(x / y);
        return x - quotient * y;
    }

    template<concepts::Integral T>
    constexpr static auto operator()(T x, T y) -> f64 {
        return Remainder::operator()(f64(x), f64(y));
    }
};
}

namespace di {
constexpr inline auto remainder = math::detail::Remainder {};
}

namespace di::math::detail {
struct Fmod {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x, T y) -> T {
        y = abs(y);
        auto result = remainder(abs(x), y);
        if (signbit(result)) {
            result += y;
        }
        return copysign(result, x);
    }

    template<concepts::Integral T>
    constexpr static auto operator()(T x, T y) -> f64 {
        return Fmod::operator()(f64(x), f64(y));
    }
};
}

namespace di {
constexpr inline auto fmod = math::detail::Fmod {};
}

namespace di::math::detail {
struct Cos {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x) -> T;

    template<concepts::Integral T>
    constexpr static auto operator()(T x) -> f64 {
        return Cos::operator()(f64(x));
    }
};

struct Sin {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x) -> T;

    template<concepts::Integral T>
    constexpr static auto operator()(T x) -> f64 {
        return Sin::operator()(f64(x));
    }
};

template<concepts::FloatingPoint T>
constexpr auto Cos::operator()(T x) -> T {
    // cos(x) is an even function.
    x = abs(x);

    // cos(x) is periodic with T = 2 pi
    x = fmod(x, 2 * numbers::pi_v<T>);
    if (x >= 3 * numbers::pi_v<T> / 2) {
        x -= 2 * numbers::pi_v<T>;
    }

    // cos(x) = -cos(x - pi)
    if (x > numbers::pi_v<T> / 2) {
        return -Cos::operator()(x - numbers::pi_v<T>);
    }

    // cos(x) is an even function.
    x = abs(x);

    // x is now in the interval [0, pi / 2]
    // Compute -sin(x - pi/2) if x > pi/4, for increased accuracy.
    if (x > numbers::pi_v<T> / 4) {
        return -Sin::operator()(x - numbers::pi_v<T> / 2);
    }

    // Use a 4 term Taylor series to compute cos(x).
    return T(1.0) - x * x / T(2.0) + x * x * x * x / T(24.0) - x * x * x * x * x * x / T(720.0);
}

template<concepts::FloatingPoint T>
constexpr auto Sin::operator()(T x) -> T {
    // sin(x) is a odd function.
    if (x < 0) {
        return -Sin::operator()(-x);
    }

    // sin(x) is periodic with T = 2 pi
    x = fmod(x, 2 * numbers::pi_v<T>);

    // sin(x) = -sin(x - pi)
    if (x > numbers::pi_v<T>) {
        return -Sin::operator()(x - numbers::pi_v<T>);
    }

    // sin(x) = sin(pi - x)
    if (x > numbers::pi_v<T> / 2) {
        x = numbers::pi_v<T> - x;
    }

    // x is now in the interval [0, pi / 2]
    // Compute cos(x - pi/2) if x > pi/4, for increased accuracy.
    if (x > numbers::pi_v<T> / 4) {
        return Cos::operator()(x - numbers::pi_v<T> / 2);
    }

    // Use a 3 term Taylor series to compute sin(x).
    return x - x * x * x / T(6.0) + x * x * x * x * x / T(120.0);
}
}

namespace di {
constexpr inline auto cos = math::detail::Cos {};
constexpr inline auto sin = math::detail::Sin {};
}

namespace di::math::detail {
struct ApproximatelyEqual {
    template<concepts::FloatingPoint T>
    constexpr static auto operator()(T x, T y, T epsilon = 0.0001) {
        return x >= y - epsilon && x <= y + epsilon;
    }
};
}

namespace di {
constexpr inline auto approximately_equal = math::detail::ApproximatelyEqual {};
}
