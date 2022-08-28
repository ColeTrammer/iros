#pragma once

#include <di/concepts/weakly_equality_comparable_with.h>

namespace di::concepts {
template<typename T>
concept EqualityComparable = detail::WeaklyEqualityComparableWith<T, T>;
}
