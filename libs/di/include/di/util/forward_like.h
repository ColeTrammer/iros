#pragma once

#include <di/meta/core.h>
#include <di/meta/util.h>

namespace di::util {
template<typename T, typename U>
[[nodiscard]] constexpr decltype(auto) forward_like(U&& value) {
    return static_cast<meta::Like<T, meta::RemoveReference<U>>&&>(value);
}
}
