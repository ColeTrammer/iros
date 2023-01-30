#pragma once

#include <di/concepts/integral.h>
#include <di/concepts/object.h>
#include <di/concepts/pointer.h>
#include <di/function/curry_back.h>
#include <di/meta/make_unsigned.h>
#include <di/meta/remove_pointer.h>
#include <di/types/prelude.h>

namespace di::math {
namespace detail {
    struct MidpointFunction {
        template<concepts::Integral T>
        requires(!concepts::SameAs<T, bool>)
        constexpr T operator()(T a, T b) const {
            // Compute the midpoint without signed overflow.
            // The implementation is from this CppCon talk:
            // https://m.youtube.com/watch?v=sBtAGxBh-XI
            using U = meta::MakeUnsigned<T>;
            int sign = 1;
            U min = a;
            U max = b;
            if (b < a) {
                sign = -1;
                min = b;
                max = a;
            }
            return a + sign * T(U(max - min) >> 1);
        }

        template<concepts::Pointer T>
        requires(concepts::Object<meta::RemovePointer<T>>)
        constexpr T operator()(T a, T b) const {
            return a + (b - a) / 2;
        }
    };
}

constexpr inline auto midpoint = function::curry_back(detail::MidpointFunction {}, meta::size_constant<2>);
}