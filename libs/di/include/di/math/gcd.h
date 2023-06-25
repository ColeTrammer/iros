#pragma once

#include <di/math/abs_unsigned.h>
#include <di/meta/common.h>
#include <di/meta/language.h>

namespace di::math {
namespace detail {
    struct GcdFunction {
        template<concepts::Integer T>
        constexpr auto operator()(T) const {
            return T(1);
        }

        template<concepts::Integer T, concepts::Integer U>
        constexpr auto operator()(T m, U n) const {
            using R = meta::CommonType<T, U>;

            if (m == 0 || n == 0) {
                return R(0);
            }

            using UR = meta::MakeUnsigned<R>;
            UR a = abs_unsigned(m);
            UR b = abs_unsigned(n);

            while (a && b) {
                if (a >= b) {
                    a = (a - b) % b;
                } else {
                    b = (b - a) % a;
                }
            }
            return a ? R(a) : R(b);
        }

        template<concepts::Integer T, concepts::Integer... Rest>
        constexpr auto operator()(T m, Rest... rest) const {
            return (*this)(m, (*this)(rest...));
        }
    };
}

constexpr inline auto gcd = detail::GcdFunction {};
}

namespace di {
using math::gcd;
}
