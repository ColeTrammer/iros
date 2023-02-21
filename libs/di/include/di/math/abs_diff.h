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

constexpr inline auto abs_diff = function::curry_back(detail::AbsDiffFunction {}, meta::size_constant<2>);
}
