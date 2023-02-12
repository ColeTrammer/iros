#pragma once

#include <di/chrono/concepts/clock.h>
#include <di/chrono/duration/duration_forward_declaration.h>
#include <di/concepts/instance_of.h>

namespace di::chrono {
template<concepts::Clock Clock, concepts::InstanceOf<Duration> Duration = typename Clock::Duration>
class TimePoint;
}