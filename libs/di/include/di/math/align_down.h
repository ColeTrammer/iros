#pragma once

#include <di/function/curry_back.h>
#include <di/meta/language.h>
#include <di/meta/util.h>

namespace di::math {
namespace detail {
    struct AlignDownFunction {
        template<concepts::Integer T>
        constexpr auto operator()(T a, meta::TypeIdentity<T> b) const -> T {
            return a / b * b;
        }
    };
}

constexpr inline auto align_down = function::curry_back(detail::AlignDownFunction {}, meta::c_<2zu>);
}

namespace di {
using math::align_down;
}
