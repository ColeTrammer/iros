#pragma once

#include <di/math/intcmp/cmp_equal.h>

namespace di::math {
namespace detail {
    struct CmpNotEqualFunction {
        template<concepts::Integer T, concepts::Integer U>
        constexpr bool operator()(T a, U b) const {
            return !cmp_equal(a, b);
        }
    };
}

constexpr inline auto cmp_not_equal = detail::CmpNotEqualFunction {};
}