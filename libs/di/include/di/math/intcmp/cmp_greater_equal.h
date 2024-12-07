#pragma once

#include "di/math/intcmp/cmp_less.h"

namespace di::math {
namespace detail {
    struct CmpGreaterEqualFunction {
        template<concepts::Integer T, concepts::Integer U>
        constexpr auto operator()(T a, U b) const -> bool {
            return !cmp_less(a, b);
        }
    };
}

constexpr inline auto cmp_greater_equal = detail::CmpGreaterEqualFunction {};
}
