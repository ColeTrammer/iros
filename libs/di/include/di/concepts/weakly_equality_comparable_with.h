#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/remove_reference.h>

namespace di::concepts::detail {
template<typename T, typename U>
concept WeaklyEqualityComparableWith = requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
    { a == b } -> SameAs<bool>;
    { a != b } -> SameAs<bool>;
    { b == a } -> SameAs<bool>;
    { b != a } -> SameAs<bool>;
};
}
