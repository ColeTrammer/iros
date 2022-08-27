#pragma once

#include <di/util/declval.h>
#include <di/util/meta/remove_reference.h>

namespace di::util::meta {
template<typename T, typename U>
using CompareThreeWayResult =
    decltype(util::declval<meta::RemoveReference<T> const&>() <=> util::declval<meta::RemoveReference<U> const&>());
}
