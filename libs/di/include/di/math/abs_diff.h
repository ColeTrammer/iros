#pragma once

#include <di/function/curry_back.h>
#include <di/meta/language.h>
#include <di/types/prelude.h>

namespace di::math {
namespace detail {
    struct AbsDiffFunction {
        template<concepts::Integral T, typename U = meta::MakeUnsigned<T>>
        constexpr auto operator()(T a, T b) const -> U {
            if (a < b) {
                return U(b) - U(a);
            }
            return U(a) - U(b);
        }

        template<concepts::Pointer T>
        requires(concepts::Object<meta::RemovePointer<T>>)
        constexpr auto operator()(T a, T b) const -> uptr {
            if (a < b) {
                return b - a;
            }
            return a - b;
        }
    };
}

constexpr inline auto abs_diff = function::curry_back(detail::AbsDiffFunction {}, meta::c_<2ZU>);
}

namespace di {
using math::abs_diff;
}
