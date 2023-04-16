#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/timer.h>

namespace iris {
void init_timer_assignments() {
    auto& global_state = global_state_in_boot();

    // Select the calibration timer, using the criteria:
    // 1. It must not have calibration itself.
    // 2. It must be support single-shot use.
    // 3. Break ties by choosing the timer with the highest resolution.
    // SAFETY: This is called during boot, so there will not be concurrent access to the timers.
    auto candidates = global_state.timers | di::filter([](di::Synchronized<Timer> const& timer) {
                          auto capabilities = timer_capabilities(timer.get_const_assuming_no_concurrent_mutations());
                          return !(capabilities & TimerCapabilities::NeedsCalibration) &&
                                 !!(capabilities & TimerCapabilities::SingleShot);
                      });
    auto it = di::max_element(candidates, [](di::Synchronized<Timer> const& a, di::Synchronized<Timer> const& b) {
        return timer_resolution(a.get_const_assuming_no_concurrent_mutations()) <=>
               timer_resolution(b.get_const_assuming_no_concurrent_mutations());
    });
    ASSERT(it != candidates.end());
    global_state.calibration_timer = &*it;
    println("Calibration timer: {}"_sv, timer_name(*global_state.calibration_timer->lock()));

    // Calibrate all timers which need it.
    // SAFETY: This is called during boot, so there will not be concurrent access to the timers.
    for (auto& timer : global_state.timers) {
        auto capabilities = timer_capabilities(timer.get_const_assuming_no_concurrent_mutations());
        if (!!(capabilities & TimerCapabilities::NeedsCalibration)) {
            auto result = timer_calibrate(timer.get_assuming_no_concurrent_accesses());
            if (!result) {
                println("Failed to calibrate timer: {}"_sv,
                        timer_name(timer.get_const_assuming_no_concurrent_mutations()));
                ASSERT(false);
            }
        }
    }

    // Choose the scheduler timer, using the criteria:
    // 1. It must support periodic use.
    // 2. If possible, it should support per-CPU use.
    // 3. Break ties by choosing the timer with the highest resolution.
    // SAFETY: This is called during boot, so there will not be concurrent access to the timers.
    auto any_scheduler_per_cpu = di::any_of(global_state.timers, [](di::Synchronized<Timer> const& timer) {
        auto capabilities = timer_capabilities(timer.get_const_assuming_no_concurrent_mutations());
        return !!(capabilities & TimerCapabilities::Periodic) && !!(capabilities & TimerCapabilities::PerCpu);
    });

    auto scheduler_candidates =
        global_state.timers | di::filter([&](di::Synchronized<Timer> const& timer) {
            auto capabilities = timer_capabilities(timer.get_const_assuming_no_concurrent_mutations());
            if (any_scheduler_per_cpu) {
                return !!(capabilities & TimerCapabilities::PerCpu) && !!(capabilities & TimerCapabilities::Periodic);
            }
            return !!(capabilities & TimerCapabilities::Periodic);
        });
    auto scheduler_it =
        di::max_element(scheduler_candidates, [](di::Synchronized<Timer> const& a, di::Synchronized<Timer> const& b) {
            return timer_resolution(a.get_const_assuming_no_concurrent_mutations()) <=>
                   timer_resolution(b.get_const_assuming_no_concurrent_mutations());
        });
    ASSERT(scheduler_it != scheduler_candidates.end());

    global_state.scheduler_timer = &*scheduler_it;
    println("Scheduler timer: {}"_sv, timer_name(*global_state.scheduler_timer->lock()));
}

di::Synchronized<Timer>& scheduler_timer() {
    return *global_state().scheduler_timer;
}

di::Synchronized<Timer>& calibration_timer() {
    return *global_state().calibration_timer;
}
}
