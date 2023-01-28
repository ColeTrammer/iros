#pragma once

#include <di/chrono/duration/duration_forward_declaration.h>
#include <di/concepts/common_with.h>

namespace di {
template<typename Rep1, math::detail::IsRatio Period1, typename Rep2, math::detail::IsRatio Period2>
requires(concepts::CommonWith<Rep1, Rep2>)
struct meta::CustomCommonType<chrono::Duration<Rep1, Period1>, chrono::Duration<Rep2, Period2>> {
    using Type =
        chrono::Duration<meta::CommonType<Rep1, Rep2>,
                         math::Ratio<math::gcd(Period1::num, Period2::num), math::lcm(Period1::den, Period2::den)>>;
};
}
