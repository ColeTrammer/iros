#include "dius/thread.h"

#include "di/chrono/duration/duration_cast.h"
#include "di/chrono/duration/duration_literals.h"
#include "dius/steady_clock.h"
#include "time.h"

namespace dius {
namespace this_thread {
    void sleep_for(di::Nanoseconds duration) {
        timespec timespec;
        timespec.tv_sec = di::duration_cast<di::Seconds>(duration).count();
        timespec.tv_nsec = duration.count() % long(di::Nanoseconds::Period::den);
        clock_nanosleep(CLOCK_MONOTONIC, 0, &timespec, nullptr);
    }

    void sleep_until(SteadyClock::TimePoint time_point) {
        timespec timespec;
        timespec.tv_sec = di::duration_cast<di::Seconds>(time_point.time_since_epoch()).count();
        timespec.tv_nsec = di::duration_cast<di::Nanoseconds>(time_point.time_since_epoch()).count() %
                           long(di::Nanoseconds::Period::den);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timespec, nullptr);
    }
}

auto Thread::do_start(di::Function<void()> entry) -> di::Result<Thread> {
    auto platform = di::make_box<PlatformThread>();
    platform->entry = di::move(entry);

    auto result = pthread_create(
        &platform->native_handle, nullptr,
        [](void* entry) -> void* {
            auto* as_function = reinterpret_cast<di::Function<void()>*>(entry);
            (*as_function)();
            return nullptr;
        },
        reinterpret_cast<void*>(&platform->entry));
    if (result != 0) {
        return di::Unexpected(PosixError(-result));
    }
    return Thread(di::move(platform));
}

auto PlatformThread::join() -> di::Result<void> {
    auto result = pthread_join(native_handle, nullptr);
    native_handle = {};
    if (result != 0) {
        return di::Unexpected(PosixError(-result));
    }
    return {};
}
}
