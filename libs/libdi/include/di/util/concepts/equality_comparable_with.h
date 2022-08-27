#pragma once

#include <di/util/concepts/equality_comparable.h>
#include <di/util/concepts/weakly_equality_comparable_with.h>

namespace di::util::concepts {
template<typename T, typename U>
concept EqualityComparableWith = EqualityComparable<T> && EqualityComparable<U> && detail::WeaklyEqualityComparableWith<T, U>;
}
