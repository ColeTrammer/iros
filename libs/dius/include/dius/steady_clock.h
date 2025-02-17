#pragma once

#include "di/chrono/duration/duration_literals.h"
#include "di/chrono/time_point/time_point.h"
#include "di/types/integers.h"

namespace dius {
class SteadyClock {
public:
    using Representation = i64;
    using Duration = di::Nanoseconds;
    using Period = Duration::Period;
    using TimePoint = di::TimePoint<SteadyClock>;

    constexpr static auto is_steady = true;

    static auto now() -> TimePoint;
};
}
