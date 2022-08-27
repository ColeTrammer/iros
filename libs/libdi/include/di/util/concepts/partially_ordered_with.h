#pragma once

#include <di/util/meta/remove_reference.h>

namespace di::util::concepts::detail {
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
