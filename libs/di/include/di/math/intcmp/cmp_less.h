#pragma once

#include <di/meta/language.h>

namespace di::math {
namespace detail {
    struct CmpLessFunction {
        template<concepts::Integer T, concepts::Integer U>
        constexpr auto operator()(T a, U b) const -> bool {
            using UT = meta::MakeUnsigned<T>;
            using UU = meta::MakeUnsigned<U>;
            if constexpr (concepts::Signed<T> == concepts::Signed<U>) {
                return a < b;
            } else if constexpr (concepts::Signed<T>) {
                return a < 0 ? true : UT(a) < b;
            } else {
                return b < 0 ? false : a < UU(b);
            }
        }
    };
}

constexpr inline auto cmp_less = detail::CmpLessFunction {};
}
