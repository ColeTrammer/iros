#pragma once

#include <di/util/meta/like.h>

namespace di::util {
template<typename T, typename U>
[[nodiscard]] constexpr meta::Like<T, U> forward_like(U&& value) {
    return static_cast<meta::Like<T, U>>(value);
}
}
