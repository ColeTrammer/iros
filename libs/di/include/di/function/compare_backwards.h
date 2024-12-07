#pragma once

#include "di/function/curry_back.h"
#include "di/math/intcmp/cmp_three_way.h"
#include "di/meta/compare.h"

namespace di::function {
struct CompareBackwards {
    template<typename T, concepts::ThreeWayComparableWith<T> U>
    constexpr auto operator()(T const& a, U const& b) const -> meta::CompareThreeWayResult<T, U> {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_three_way(b, a);
        } else {
            return b <=> a;
        }
    }
};

constexpr inline auto compare_backwards = function::curry_back(CompareBackwards {}, meta::c_<2ZU>);
}

namespace di {
using function::compare_backwards;
using function::CompareBackwards;
}
