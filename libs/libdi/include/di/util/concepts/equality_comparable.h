#pragma once

#include <di/util/concepts/weakly_equality_comparable_with.h>

namespace di::util::concepts {
template<typename T>
concept EqualityComparable = detail::WeaklyEqualityComparableWith<T, T>;
}
