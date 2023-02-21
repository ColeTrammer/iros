#pragma once

#include <di/concepts/unsigned_integer.h>
#include <di/math/numeric_limits.h>

namespace di::bit {
namespace detail {
    struct RotlFunction {
        template<concepts::UnsignedInteger T>
        constexpr T operator()(T x, int s) const {
            constexpr auto N = math::NumericLimits<T>::digits;
            int r = s % N;
            if (r == 0) {
                return x;
            } else if (r > 0) {
                return (x << r) | (x >> (N - r));
            } else {
                return (x >> -r) | (x << (N + r));
            }
        }
    };
}

constexpr inline auto rotl = detail::RotlFunction {};
}
