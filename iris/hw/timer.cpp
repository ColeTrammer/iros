#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/hw/timer.h>

namespace iris {
void init_timer_assignments() {
    auto& global_state = global_state_in_boot();
    global_state.scheduler_timer = global_state.timers.front().data();
    global_state.calibration_timer = global_state.timers.front().data();

    println("Scheduler timer: {}"_sv, timer_name(*global_state.scheduler_timer->lock()));
    println("Calibration timer: {}"_sv, timer_name(*global_state.calibration_timer->lock()));
}

di::Synchronized<Timer>& scheduler_timer() {
    return *global_state().scheduler_timer;
}

di::Synchronized<Timer>& calibration_timer() {
    return *global_state().calibration_timer;
}
}
