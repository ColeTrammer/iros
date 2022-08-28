#pragma once

#include <di/concepts/partially_ordered_with.h>
#include <di/concepts/weakly_equality_comparable_with.h>
#include <di/concepts/weakly_three_way_comparable_with.h>

namespace di::concepts {
template<typename T, typename Category = types::partial_ordering>
concept ThreeWayComparable = detail::WeaklyEqualityComparableWith<T, T> && detail::PartiallyOrderedWith<T, T> &&
                             detail::WeaklyThreeWayComparableWith<T, T, Category>;
}
