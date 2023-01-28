#pragma once

#include <di/concepts/three_way_comparable_with.h>
#include <di/types/weak_ordering.h>

namespace di::concepts {
template<typename T, typename U>
concept TotallyOrderedWith = ThreeWayComparableWith<T, U, types::weak_ordering>;
}
