#pragma once

#include <di/util/meta/like.h>
#include <di/util/meta/remove_reference.h>

namespace di::util {
template<typename T, typename U>
[[nodiscard]] constexpr decltype(auto) forward_like(U&& value) {
    return static_cast<meta::Like<T, meta::RemoveReference<U>>&&>(value);
}
}
