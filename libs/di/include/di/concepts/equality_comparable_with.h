#pragma once

#include <di/concepts/equality_comparable.h>
#include <di/concepts/weakly_equality_comparable_with.h>

namespace di::concepts {
template<typename T, typename U>
concept EqualityComparableWith =
    detail::DefinitelyEqualityComparableWith<T, U>::value ||
    (EqualityComparable<T> && EqualityComparable<U> && detail::WeaklyEqualityComparableWith<T, U>);
}
