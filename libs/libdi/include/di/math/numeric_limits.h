#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/unsigned_integer.h>

namespace di::math {
template<concepts::Integer T>
struct NumericLimits;

template<concepts::UnsignedInteger T>
struct NumericLimits<T> {
    constexpr static T min = 0;
    constexpr static T max = static_cast<T>(-1);
    constexpr static T bits = sizeof(T) * 8u;
};
}