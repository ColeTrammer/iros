#pragma once

#include <di/concepts/integer.h>
#include <di/meta/make_unsigned.h>

namespace di::math {
namespace detail {
    struct CmpEqualFunction {
        template<concepts::Integer T, concepts::Integer U>
        constexpr bool operator()(T a, U b) const {
            using UT = meta::MakeUnsigned<T>;
            using UU = meta::MakeUnsigned<U>;
            if constexpr (concepts::Signed<T> == concepts::Signed<U>) {
                return a == b;
            } else if constexpr (concepts::Signed<T>) {
                return a < 0 ? false : UT(a) == b;
            } else {
                return b < 0 ? false : a == UU(b);
            }
        }
    };
}

constexpr inline auto cmp_equal = detail::CmpEqualFunction {};
}
