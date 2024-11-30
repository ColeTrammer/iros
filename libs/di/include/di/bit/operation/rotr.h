#pragma once

#include <di/math/numeric_limits.h>
#include <di/meta/language.h>

namespace di::bit {
namespace detail {
    struct RotrFunction {
        template<concepts::UnsignedInteger T>
        constexpr auto operator()(T x, int s) const -> T {
            constexpr auto N = math::NumericLimits<T>::digits;
            int r = s % N;
            if (r == 0) {
                return x;
            }
            if (r > 0) {
                return (x >> r) | (x << (N - r));
            }
            return (x << -r) | (x >> (N + r));
        }
    };
}

constexpr inline auto rotr = detail::RotrFunction {};
}

namespace di {
using bit::rotr;
}
