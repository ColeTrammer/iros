#pragma once

#include <di/util/concepts/partially_ordered_with.h>
#include <di/util/concepts/three_way_comparable.h>
#include <di/util/concepts/weakly_equality_comparable_with.h>
#include <di/util/concepts/weakly_three_way_comparable_with.h>

namespace di::util::concepts {
template<typename T, typename U, typename Category = types::partial_ordering>
concept ThreeWayComparableWith = ThreeWayComparable<T> && ThreeWayComparable<U> && detail::WeaklyEqualityComparableWith<T, U> &&
                                 detail::PartiallyOrderedWith<T, U> && detail::WeaklyThreeWayComparableWith<T, U, Category>;
}
