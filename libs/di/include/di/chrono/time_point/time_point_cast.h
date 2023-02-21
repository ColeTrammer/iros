#pragma once

#include <di/chrono/duration/duration_cast.h>
#include <di/chrono/time_point/time_point_forward_declaration.h>
#include <di/concepts/instance_of.h>

namespace di::chrono {
namespace detail {
    template<concepts::InstanceOf<Duration> To>
    struct TimePointCastFunction {
        template<typename Clock, typename Duration>
        constexpr TimePoint<Clock, To> operator()(TimePoint<Clock, Duration> const& from) const {
            return TimePoint<Clock, To>(chrono::duration_cast<To>(from.time_since_epoch()));
        }
    };
}

template<concepts::InstanceOf<Duration> To>
constexpr inline auto time_point_cast = detail::TimePointCastFunction<To> {};
}
