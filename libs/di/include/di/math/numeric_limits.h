#pragma once

#include "di/meta/language.h"

namespace di::math {
template<typename T>
struct NumericLimits;

template<concepts::UnsignedInteger T>
struct NumericLimits<T> {
    constexpr static T max = static_cast<T>(-1);
    constexpr static T min = 0;
    constexpr static int bits = sizeof(T) * 8;
    constexpr static int digits = bits;
};

template<concepts::SignedInteger T>
struct NumericLimits<T> {
    using Unsigned = meta::MakeUnsigned<T>;

    constexpr static T max = static_cast<T>(static_cast<Unsigned>(-1) >> static_cast<Unsigned>(1));
    constexpr static T min = ~max;
    constexpr static int bits = sizeof(T) * 8;
    constexpr static int digits = bits - 1;
};

template<>
struct NumericLimits<float> {
    constexpr static auto quiet_nan = __builtin_nanf("");
    constexpr static auto signaling_nan = __builtin_nansf("");
    constexpr static auto infinity = __builtin_huge_valf();
};

template<>
struct NumericLimits<double> {
    constexpr static auto quiet_nan = __builtin_nan("");
    constexpr static auto signaling_nan = __builtin_nans("");
    constexpr static auto infinity = __builtin_huge_val();
};

template<>
struct NumericLimits<long double> {
    constexpr static auto quiet_nan = __builtin_nanl("");
    constexpr static auto signaling_nan = __builtin_nansl("");
    constexpr static auto infinity = __builtin_huge_vall();
};
}

namespace di {
using math::NumericLimits;
}
