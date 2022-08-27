#pragma once

#include <di/util/concepts/compares_as.h>
#include <di/util/meta/remove_reference.h>

namespace di::util::concepts::detail {
template<typename T, typename U, typename Category>
concept WeaklyThreeWayComparableWith = requires(meta::RemoveReference<T> const& a, meta::RemoveReference<U> const& b) {
                                           { a <=> b } -> ComparesAs<Category>;
                                           { b <=> a } -> ComparesAs<Category>;
                                       };
}
