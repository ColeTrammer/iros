#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/preemption.h>
#include <iris/core/print.h>

namespace iris {
PreemptionDisabler::PreemptionDisabler() {
    // Disable interrupts to prevent any context switches from occuring.
    auto guard = InterruptDisabler {};

    // SAFETY: This is safe since interrupts are disabled.
    auto& scheduler = current_processor_unsafe().scheduler();

    m_task = scheduler.current_task_null_if_during_boot();
    if (m_task) {
        m_task->disable_preemption();
    }
}

PreemptionDisabler::~PreemptionDisabler() {
    if (!m_task) {
        return;
    }
    m_task->enable_preemption();
}
}
