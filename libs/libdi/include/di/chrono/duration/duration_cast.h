#pragma once

#include <di/chrono/duration/duration_forward_declaration.h>
#include <di/concepts/instance_of.h>

namespace di::chrono {
template<concepts::InstanceOf<Duration> To, typename Rep, math::detail::IsRatio Period>
constexpr To duration_cast(Duration<Rep, Period> const&) {
    return To {};
}
}