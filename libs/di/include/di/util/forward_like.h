#pragma once

#include <di/meta/core.h>
#include <di/meta/util.h>

namespace di::util {
template<typename T, typename U>
[[nodiscard]] constexpr auto forward_like(U&& value) -> decltype(auto) {
    return static_cast<meta::Like<T, meta::RemoveReference<U>>&&>(value);
}
}

namespace di {
using util::forward_like;
}
