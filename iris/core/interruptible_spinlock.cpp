#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/interruptible_spinlock.h>
#include <iris/core/task.h>

namespace iris {
void InterruptibleSpinlock::lock() {
    for (;;) {
        if (try_lock()) {
            return;
        }
        while (m_state.load(di::sync::MemoryOrder::Relaxed)) {
            di::cpu_relax();
        }
    }
}

bool InterruptibleSpinlock::try_lock() {
    auto* task = with_interrupts_disabled([] {
        // SAFETY: This is safe since interrupts are disabled.
        return current_processor_unsafe().scheduler().current_task_null_if_during_boot();
    });
    if (task) {
        task->disable_preemption();
    }
    auto result = !m_state.exchange(true, di::sync::MemoryOrder::Acquire);
    if (!result) {
        if (task) {
            task->enable_preemption();
        }
        return false;
    }
    m_task = task;
    return true;
}

void InterruptibleSpinlock::unlock() {
    auto current_task = m_task;
    m_state.store(false, di::sync::MemoryOrder::Release);
    if (current_task) {
        current_task->enable_preemption();
    }
}
}
