#pragma once

#include <di/math/rational/rational.h>
#include <di/types/prelude.h>

namespace di::math {
template<i128 numerator, i128 denominator = 1>
struct Ratio;

namespace detail {
    template<typename T>
    struct IsRatioHelper : meta::BoolConstant<false> {};

    template<i128 x, i128 y>
    struct IsRatioHelper<Ratio<x, y>> : meta::BoolConstant<true> {};

    template<typename T>
    concept IsRatio = IsRatioHelper<T>::value;
}

template<Rational<i128> rational>
using RatioFromRational = Ratio<rational.numerator(), rational.denominator()>;

template<i128 numerator, i128 denominator>
struct Ratio {
    constexpr static i128 num = numerator;
    constexpr static i128 den = denominator;

    constexpr static Rational<i128> rational { num, den };

    using Type = RatioFromRational<rational>;
};

template<detail::IsRatio A, detail::IsRatio B>
using RatioAdd = RatioFromRational<A::rational + B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioSubtract = RatioFromRational<A::rational - B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioMultiply = RatioFromRational<A::rational * B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioDivide = RatioFromRational<A::rational / B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioEqual = meta::BoolConstant<A::rational == B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioNotEqual = meta::BoolConstant<A::rational != B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioLess = meta::BoolConstant<(A::rational < B::rational)>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioLessEqual = meta::BoolConstant<(A::rational <= B::rational)>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioGreater = meta::BoolConstant<(A::rational > B::rational)>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioGreaterEqual = meta::BoolConstant<(A::rational >= B::rational)>;

using Yocto = Ratio<1, i128(1000000000000000000) * i128(1000000)>;
using Zepto = Ratio<1, i128(1000000000000000000) * i128(1000)>;
using Atto = Ratio<1, 1000000000000000000>;
using Femto = Ratio<1, 1000000000000000>;
using Pico = Ratio<1, 1000000000000>;
using Nano = Ratio<1, 1000000000>;
using Micro = Ratio<1, 1000000>;
using Milli = Ratio<1, 1000>;
using Centi = Ratio<1, 100>;
using Deci = Ratio<1, 10>;
using Deca = Ratio<10>;
using Hecto = Ratio<100>;
using Kilo = Ratio<1000>;
using Mega = Ratio<1000000>;
using Giga = Ratio<1000000000>;
using Tera = Ratio<1000000000000>;
using Peta = Ratio<1000000000000000>;
using Exa = Ratio<1000000000000000000>;
using Yetta = Ratio<i128(1000000000000000000) * i128(1000)>;
using Yotta = Ratio<i128(1000000000000000000) * i128(1000000)>;
}