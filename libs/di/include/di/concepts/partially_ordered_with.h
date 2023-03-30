#pragma once

#include <di/meta/remove_reference.h>

namespace di::concepts::detail {
template<typename T, typename U>
concept PartiallyOrderedWith = requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
    { a < b } -> SameAs<bool>;
    { a > b } -> SameAs<bool>;
    { a <= b } -> SameAs<bool>;
    { a >= b } -> SameAs<bool>;
    { b < a } -> SameAs<bool>;
    { b > a } -> SameAs<bool>;
    { b <= a } -> SameAs<bool>;
    { b >= a } -> SameAs<bool>;
};
}
