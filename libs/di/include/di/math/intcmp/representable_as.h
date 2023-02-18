#pragma once

#include <di/math/intcmp/cmp_greater_equal.h>
#include <di/math/intcmp/cmp_less_equal.h>
#include <di/math/numeric_limits.h>

namespace di::math {
namespace detail {
    template<concepts::Integer T>
    struct RepresentableAsFunction {
        template<concepts::Integer U>
        constexpr bool operator()(U value) const {
            return cmp_greater_equal(value, NumericLimits<T>::min) && cmp_less_equal(value, NumericLimits<T>::max);
        }
    };
}

template<concepts::Integer T>
constexpr inline auto representable_as = detail::RepresentableAsFunction<T> {};
}