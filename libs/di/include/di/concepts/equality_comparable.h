#pragma once

#include <di/concepts/definitely_equality_comparable_with.h>
#include <di/concepts/weakly_equality_comparable_with.h>

namespace di::concepts {
template<typename T>
concept EqualityComparable =
    detail::DefinitelyEqualityComparableWith<T, T>::value || detail::WeaklyEqualityComparableWith<T, T>;
}
