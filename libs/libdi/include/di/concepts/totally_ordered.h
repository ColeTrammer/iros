#pragma once

#include <di/concepts/three_way_comparable.h>
#include <di/types/weak_ordering.h>

namespace di::concepts {
template<typename T>
concept TotallyOrdered = ThreeWayComparable<T, types::weak_ordering>;
}
