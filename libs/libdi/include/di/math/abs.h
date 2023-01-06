#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/signed_integer.h>

namespace di::math {
template<concepts::Integral T>
constexpr T abs(T value) {
    if constexpr (concepts::SignedIntegral<T>) {
        if (value < 0) {
            return -value;
        }
    }
    return value;
}
}