#pragma once

#include <di/math/intcmp/cmp_greater_equal.h>
#include <di/math/intcmp/cmp_less_equal.h>
#include <di/math/numeric_limits.h>

namespace di::math {
namespace detail {
    template<concepts::Integer T>
    struct RepresentableAsFunction {
        template<concepts::Integer U>
        constexpr auto operator()(U value) const -> bool {
            return cmp_greater_equal(value, NumericLimits<T>::min) && cmp_less_equal(value, NumericLimits<T>::max);
        }
    };
}

template<concepts::Integer T>
constexpr inline auto representable_as = detail::RepresentableAsFunction<T> {};
}

namespace di {
using math::representable_as;
}
