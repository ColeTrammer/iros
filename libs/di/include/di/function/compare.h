#pragma once

#include <di/function/curry_back.h>
#include <di/math/intcmp/cmp_three_way.h>
#include <di/meta/compare.h>

namespace di::function {
struct Compare {
    template<typename T, concepts::ThreeWayComparableWith<T> U>
    constexpr meta::CompareThreeWayResult<T, U> operator()(T const& a, U const& b) const {
        if constexpr (concepts::Integer<T> && concepts::Integer<U>) {
            return math::cmp_three_way(a, b);
        } else {
            return a <=> b;
        }
    }
};

constexpr inline auto compare = function::curry_back(Compare {}, meta::c_<2zu>);
}

namespace di {
using function::compare;
using function::Compare;
}
