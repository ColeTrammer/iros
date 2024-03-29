#pragma once

#include <di/chrono/duration/prelude.h>
#include <di/util/prelude.h>
#include <iris/core/error.h>
#include <iris/hw/irq.h>

namespace iris {
enum class TimerCapabilities {
    SingleShot = 1 << 0,
    Periodic = 1 << 1,
    PerCpu = 1 << 2,
    NeedsCalibration = 1 << 3,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(TimerCapabilities)

using TimerResolution = di::Picoseconds;

namespace detail {
    template<typename R = void>
    struct TimerDefaultNotSupported {
        inline Expected<R> operator()(auto&&...) const { return di::Unexpected(Error::NotSupported); }
    };
}

struct TimerName : di::Dispatcher<TimerName, di::StringView(di::This const&)> {};
struct TimerCapabilitiesFunction : di::Dispatcher<TimerCapabilitiesFunction, TimerCapabilities(di::This const&)> {};
struct TimerResolutionFunction : di::Dispatcher<TimerResolutionFunction, TimerResolution(di::This const&)> {};
struct TimerCalibrateFunction
    : di::Dispatcher<TimerCalibrateFunction, Expected<void>(di::This&), detail::TimerDefaultNotSupported<>> {};

struct TimerSetSingleShotFunction
    : di::Dispatcher<TimerSetSingleShotFunction,
                     Expected<void>(di::This&, TimerResolution, di::Function<void(IrqContext&)>),
                     detail::TimerDefaultNotSupported<>> {};
struct TimerSetIntervalFunction
    : di::Dispatcher<TimerSetIntervalFunction,
                     Expected<void>(di::This&, TimerResolution, di::Function<void(IrqContext&)>),
                     detail::TimerDefaultNotSupported<>> {};

constexpr inline auto timer_name = TimerName {};
constexpr inline auto timer_capabilities = TimerCapabilitiesFunction {};
constexpr inline auto timer_resolution = TimerResolutionFunction {};
constexpr inline auto timer_calibrate = TimerCalibrateFunction {};
constexpr inline auto timer_set_single_shot = TimerSetSingleShotFunction {};
constexpr inline auto timer_set_interval = TimerSetIntervalFunction {};

using TimerInterface = di::meta::List<TimerName, TimerCapabilitiesFunction, TimerResolutionFunction,
                                      TimerCalibrateFunction, TimerSetSingleShotFunction, TimerSetIntervalFunction>;
using Timer = di::Any<TimerInterface>;

di::Synchronized<Timer>& scheduler_timer();
di::Synchronized<Timer>& calibration_timer();

void init_timer_assignments();
}
