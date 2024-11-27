#pragma once

#include <di/chrono/duration/duration_forward_declaration.h>
#include <di/meta/core.h>

namespace di::chrono {
namespace detail {
    template<concepts::InstanceOf<Duration> To>
    struct DurationCastFunction {
        template<typename Rep, math::detail::IsRatio Period>
        constexpr auto operator()(Duration<Rep, Period> const& from) const -> To {
            constexpr auto conversion_factor = math::RatioDivide<Period, typename To::Period>::rational;

            auto count = static_cast<imax>(from.count());
            count *= conversion_factor.numerator();
            count /= conversion_factor.denominator();

            using Rep2 = To::Representation;
            return To(static_cast<Rep2>(count));
        }
    };
}

template<concepts::InstanceOf<Duration> To>
constexpr inline auto duration_cast = detail::DurationCastFunction<To> {};
}

namespace di {
using chrono::duration_cast;
}
