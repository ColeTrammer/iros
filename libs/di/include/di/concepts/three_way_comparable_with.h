#pragma once

#include <di/concepts/partially_ordered_with.h>
#include <di/concepts/three_way_comparable.h>
#include <di/concepts/weakly_equality_comparable_with.h>
#include <di/concepts/weakly_three_way_comparable_with.h>

namespace di::concepts {
template<typename T, typename U, typename Category = types::partial_ordering>
concept ThreeWayComparableWith =
    ThreeWayComparable<T> && ThreeWayComparable<U> && detail::WeaklyEqualityComparableWith<T, U> &&
    detail::PartiallyOrderedWith<T, U> && detail::WeaklyThreeWayComparableWith<T, U, Category>;
}
