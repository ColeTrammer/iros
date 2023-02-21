#pragma once

#include <di/concepts/enum.h>
#include <di/meta/underlying_type.h>

namespace di::util {
template<concepts::Enum T>
constexpr auto to_underlying(T value) {
    return static_cast<meta::UnderlyingType<T>>(value);
}
}
