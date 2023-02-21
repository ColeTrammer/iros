#pragma once

#include <di/math/intcmp/cmp_less.h>

namespace di::math {
namespace detail {
    struct CmpGreaterFunction {
        template<concepts::Integer T, concepts::Integer U>
        constexpr bool operator()(T a, U b) const {
            return cmp_less(b, a);
        }
    };
}

constexpr inline auto cmp_greater = detail::CmpGreaterFunction {};
}
