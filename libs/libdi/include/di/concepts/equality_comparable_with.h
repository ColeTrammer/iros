#pragma once

#include <di/concepts/equality_comparable.h>
#include <di/concepts/weakly_equality_comparable_with.h>

namespace di::concepts {
template<typename T, typename U>
concept EqualityComparableWith =
    EqualityComparable<T> && EqualityComparable<U> && detail::WeaklyEqualityComparableWith<T, U>;
}
