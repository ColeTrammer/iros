#pragma once

#include <di/concepts/compares_as.h>
#include <di/concepts/definitely_three_way_comparable_with.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/partially_ordered_with.h>
#include <di/concepts/weakly_equality_comparable_with.h>
#include <di/concepts/weakly_three_way_comparable_with.h>
#include <di/meta/list/type.h>

namespace di::concepts {
template<typename T, typename Category = types::partial_ordering>
concept ThreeWayComparable =
    (detail::ComparesAs<meta::Type<detail::DefinitelyThreeWayComparableWith<T, T>>, Category>) ||
    (detail::WeaklyEqualityComparableWith<T, T> && detail::PartiallyOrderedWith<T, T> &&
     detail::WeaklyThreeWayComparableWith<T, T, Category>);
}
