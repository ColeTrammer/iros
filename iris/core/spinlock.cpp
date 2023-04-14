#include <iris/core/print.h>
#include <iris/core/spinlock.h>

namespace iris {
void Spinlock::lock() {
    for (;;) {
        if (try_lock()) {
            return;
        }
        while (m_state.load(di::sync::MemoryOrder::Relaxed)) {
            di::cpu_relax();
        }
    }
}

bool Spinlock::try_lock() {
    auto interrupts_were_disabled = raw_disable_interrupts_and_save_previous_state();
    auto result = !m_state.exchange(true, di::sync::MemoryOrder::Acquire);
    if (!result) {
        if (!interrupts_were_disabled) {
            raw_enable_interrupts();
        }
        return false;
    }
    m_interrupts_were_disabled = interrupts_were_disabled;
    return true;
}

void Spinlock::unlock() {
    auto interrupts_were_disabled = m_interrupts_were_disabled;
    m_state.store(false, di::sync::MemoryOrder::Release);
    if (!interrupts_were_disabled) {
        raw_enable_interrupts();
    }
}
}
