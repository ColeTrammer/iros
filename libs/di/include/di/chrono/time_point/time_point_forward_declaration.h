#pragma once

#include <di/chrono/duration/duration_forward_declaration.h>
#include <di/concepts/instance_of.h>

namespace di::chrono {
template<typename Clock, concepts::InstanceOf<Duration> Duration = typename Clock::Duration>
class TimePoint;
}
