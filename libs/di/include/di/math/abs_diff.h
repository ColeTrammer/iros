#pragma once

#include <di/function/curry_back.h>
#include <di/meta/language.h>
#include <di/types/prelude.h>

namespace di::math {
namespace detail {
    struct AbsDiffFunction {
        template<concepts::Integral T, typename U = meta::MakeUnsigned<T>>
        constexpr U operator()(T a, T b) const {
            if (a < b) {
                return U(b) - U(a);
            } else {
                return U(a) - U(b);
            }
        }

        template<concepts::Pointer T>
        requires(concepts::Object<meta::RemovePointer<T>>)
        constexpr uptr operator()(T a, T b) const {
            if (a < b) {
                return b - a;
            } else {
                return a - b;
            }
        }
    };
}

constexpr inline auto abs_diff = function::curry_back(detail::AbsDiffFunction {}, meta::c_<2zu>);
}

namespace di {
using math::abs_diff;
}
