#pragma once

#include <di/concepts/three_way_comparable_with.h>
#include <di/function/curry_back.h>
#include <di/meta/compare_three_way_result.h>

namespace di::function {
struct CompareBackwards {
    template<typename T, concepts::ThreeWayComparableWith<T> U>
    constexpr meta::CompareThreeWayResult<T, U> operator()(T const& a, U const& b) const {
        return b <=> a;
    }
};

constexpr inline auto compare_backwards = function::curry_back(CompareBackwards {}, meta::size_constant<2>);
}