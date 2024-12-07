#pragma once

#include "di/meta/language.h"
#include "di/types/strong_ordering.h"

namespace di::math {
namespace detail {
    struct CmpThreeWayFunction {
        template<concepts::Integer T, concepts::Integer U>
        constexpr auto operator()(T a, U b) const -> types::strong_ordering {
            using UT = meta::MakeUnsigned<T>;
            using UU = meta::MakeUnsigned<U>;
            if constexpr (concepts::Signed<T> == concepts::Signed<U>) {
                return a <=> b;
            } else if constexpr (concepts::Signed<T>) {
                return a < 0 ? types::strong_ordering::less : UT(a) <=> b;
            } else {
                return b < 0 ? types::strong_ordering::greater : a <=> UU(b);
            }
        }
    };
}

constexpr inline auto cmp_three_way = detail::CmpThreeWayFunction {};
}
