#pragma once

#include "di/function/curry_back.h"
#include "di/function/equal_or_greater.h"
#include "di/function/equal_or_less.h"

namespace di::function {
struct BetweenInclusive : CurryBack<BetweenInclusive> {
    template<typename T, typename U = T, typename V = T>
    constexpr static auto operator()(T const& value, U const& lower, V const& upper) -> bool
    requires(requires {
        di::equal_or_greater(value, lower);
        di::equal_or_less(value, upper);
    })
    {
        return di::equal_or_greater(value, lower) && di::equal_or_less(value, upper);
    }

    using CurryBack<BetweenInclusive>::operator();
    constexpr static auto max_arity = 3ZU;
};

constexpr inline auto between_inclusive = BetweenInclusive {};
}

namespace di {
using function::between_inclusive;
using function::BetweenInclusive;
}
