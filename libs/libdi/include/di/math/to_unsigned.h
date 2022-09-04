#pragma once

#include <di/concepts/integral.h>
#include <di/meta/make_unsigned.h>

namespace di::math {
template<concepts::Integral T>
constexpr auto to_unsigned(T value) {
    return static_cast<meta::MakeUnsigned<T>>(value);
}
}
