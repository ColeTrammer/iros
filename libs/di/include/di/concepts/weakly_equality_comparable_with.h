#pragma once

#include <di/concepts/definitely_equality_comparable_with.h>
#include <di/meta/core.h>
#include <di/meta/remove_reference.h>

namespace di::concepts::detail {
template<typename T, typename U>
concept WeaklyEqualityComparableWith = (detail::DefinitelyEqualityComparableWith<T, U>::value) ||
                                       requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
                                           { a == b } -> SameAs<bool>;
                                           { a != b } -> SameAs<bool>;
                                           { b == a } -> SameAs<bool>;
                                           { b != a } -> SameAs<bool>;
                                       };
}
