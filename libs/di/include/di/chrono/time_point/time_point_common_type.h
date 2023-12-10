#pragma once

#include <di/chrono/concepts/clock.h>
#include <di/chrono/duration/duration_common_type.h>
#include <di/chrono/time_point/time_point_forward_declaration.h>

namespace di {
template<concepts::Clock Clock, typename D1, typename D2>
requires(concepts::CommonWith<D1, D2>)
struct meta::CustomCommonType<chrono::TimePoint<Clock, D1>, chrono::TimePoint<Clock, D2>> {
    using Type = chrono::TimePoint<Clock, meta::CommonType<D1, D2>>;
};
}
