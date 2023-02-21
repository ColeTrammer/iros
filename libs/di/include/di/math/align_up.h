#pragma once

#include <di/concepts/integer.h>
#include <di/function/curry_back.h>
#include <di/meta/type_identity.h>

namespace di::math {
namespace detail {
    struct AlignUpFunction {
        template<concepts::Integer T>
        constexpr T operator()(T a, meta::TypeIdentity<T> b) const {
            return (a + b - 1) / b * b;
        }
    };
}

constexpr inline auto align_up = function::curry_back(detail::AlignUpFunction {}, meta::size_constant<2>);
}
