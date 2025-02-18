#include "dius/steady_clock.h"

#include <linux/time.h>

#include "dius/system/system_call.h"

namespace dius {
auto SteadyClock::now() -> TimePoint {
    timespec timespec;

    // TODO: use the vdso instead.
    (void) system::system_call<i32>(system::Number::clock_gettime, CLOCK_MONOTONIC, &timespec);

    return TimePoint(di::Seconds(timespec.tv_sec) + di::Nanoseconds(timespec.tv_nsec));
}
}
