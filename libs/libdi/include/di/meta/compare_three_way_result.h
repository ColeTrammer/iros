#pragma once

#include <di/meta/remove_reference.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T, typename U = T>
using CompareThreeWayResult =
    decltype(util::declval<meta::RemoveReference<T> const&>() <=> util::declval<meta::RemoveReference<U> const&>());
}
