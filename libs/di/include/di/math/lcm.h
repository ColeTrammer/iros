#pragma once

#include <di/math/gcd.h>

namespace di::math {
namespace detail {
    struct LcmFunction {
        template<concepts::Integer T>
        constexpr auto operator()(T m) const {
            return m;
        }

        template<concepts::Integer T, concepts::Integer U>
        constexpr auto operator()(T m, U n) const {
            using R = meta::CommonType<T, U>;
            if (m == 0 || n == 0) {
                return R(0);
            }

            using UR = meta::MakeUnsigned<R>;
            UR a = math::abs_unsigned(m);
            UR b = math::abs_unsigned(n);

            return R(a * (b / gcd(a, b)));
        }

        template<concepts::Integer T, concepts::Integer... Rest>
        constexpr auto operator()(T m, Rest... rest) const {
            return (*this)(m, (*this)(rest...));
        }
    };
}

constexpr inline auto lcm = detail::LcmFunction {};
}