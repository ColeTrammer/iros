#pragma once

#include <di/sync/atomic.h>
#include <di/util/immovable.h>
#include <iris/core/interrupt_disabler.h>

namespace iris {
class Spinlock : di::util::Immovable {
public:
    Spinlock() = default;

    void lock() {
        for (;;) {
            if (try_lock()) {
                return;
            }
            while (m_state.load(di::sync::MemoryOrder::Relaxed)) {
#ifdef __x86_64__
                asm volatile("pause" ::: "memory");
#endif
            }
        }
    }

    bool try_lock() {
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

    void unlock() {
        auto interrupts_were_disabled = m_interrupts_were_disabled;
        m_state.store(false, di::sync::MemoryOrder::Release);
        if (!interrupts_were_disabled) {
            raw_enable_interrupts();
        }
    }

private:
    di::sync::Atomic<bool> m_state { false };
    bool m_interrupts_were_disabled { false };
};
}
