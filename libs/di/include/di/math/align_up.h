#pragma once

#include <di/function/curry_back.h>
#include <di/meta/language.h>
#include <di/meta/util.h>

namespace di::math {
namespace detail {
    struct AlignUpFunction {
        template<concepts::Integer T>
        constexpr T operator()(T a, meta::TypeIdentity<T> b) const {
            return (a + b - 1) / b * b;
        }
    };
}

constexpr inline auto align_up = function::curry_back(detail::AlignUpFunction {}, meta::c_<2zu>);
}
