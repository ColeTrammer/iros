#pragma once

#include <di/concepts/integer.h>
#include <di/concepts/unsigned_integer.h>
#include <di/meta/make_unsigned.h>

namespace di::math {
template<concepts::Integer T>
struct NumericLimits;

template<concepts::UnsignedInteger T>
struct NumericLimits<T> {
    constexpr static T max = static_cast<T>(-1);
    constexpr static T min = 0;
    constexpr static T bits = sizeof(T) * 8u;
};

template<concepts::SignedInteger T>
struct NumericLimits<T> {
    using Unsigned = meta::MakeUnsigned<T>;

    constexpr static T max = static_cast<T>(static_cast<Unsigned>(-1) >> static_cast<Unsigned>(1));
    constexpr static T min = ~max;
    constexpr static T bits = sizeof(T) * 8;
};
}