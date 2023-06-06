#pragma once

#include <di/math/rational/rational.h>
#include <di/meta/constexpr.h>
#include <di/types/prelude.h>

namespace di::math {
#ifdef DI_HAVE_128_BIT_INTEGERS
using ratio_intmax_t = i128;
#else
using ratio_intmax_t = i64;
#endif

template<ratio_intmax_t numerator, ratio_intmax_t denominator = 1>
struct Ratio;

namespace detail {
    template<typename T>
    constexpr inline bool is_ratio = false;

    template<ratio_intmax_t x, ratio_intmax_t y>
    constexpr inline bool is_ratio<Ratio<x, y>> = true;

    template<typename T>
    concept IsRatio = is_ratio<T>;
}

template<Rational<ratio_intmax_t> rational>
using RatioFromRational = Ratio<rational.numerator(), rational.denominator()>;

template<ratio_intmax_t numerator, ratio_intmax_t denominator>
struct Ratio {
    constexpr static ratio_intmax_t num = numerator;
    constexpr static ratio_intmax_t den = denominator;

    constexpr static Rational<ratio_intmax_t> rational { num, den };

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
using RatioEqual = Constexpr<A::rational == B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioNotEqual = Constexpr<A::rational != B::rational>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioLess = Constexpr<(A::rational < B::rational)>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioLessEqual = Constexpr<(A::rational <= B::rational)>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioGreater = Constexpr<(A::rational > B::rational)>;

template<detail::IsRatio A, detail::IsRatio B>
using RatioGreaterEqual = Constexpr<(A::rational >= B::rational)>;

#ifdef DI_HAVE_128_BIT_INTEGERS
using Yocto = Ratio<1, ratio_intmax_t(1000000000000000000) * ratio_intmax_t(1000000)>;
using Zepto = Ratio<1, ratio_intmax_t(1000000000000000000) * ratio_intmax_t(1000)>;
#endif
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
#ifdef DI_HAVE_128_BIT_INTEGERS
using Yetta = Ratio<ratio_intmax_t(1000000000000000000) * ratio_intmax_t(1000)>;
using Yotta = Ratio<ratio_intmax_t(1000000000000000000) * ratio_intmax_t(1000000)>;
#endif
}
