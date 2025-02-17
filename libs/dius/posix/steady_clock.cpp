#include "dius/steady_clock.h"

#include <time.h>

namespace dius {
auto SteadyClock::now() -> TimePoint {
    timespec timespec;
    clock_gettime(CLOCK_MONOTONIC, &timespec);

    return TimePoint(di::Seconds(timespec.tv_sec) + di::Nanoseconds(timespec.tv_nsec));
}
}
