#pragma once

#include <iris/core/global_state.h>
#include <iris/core/preemption.h>

namespace iris {
class InterruptibleSpinlock : di::util::Immovable {
public:
    InterruptibleSpinlock() = default;

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
        auto* task = current_scheduler().current_task_null_if_during_boot();
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

    void unlock() {
        auto current_task = m_task;
        m_state.store(false, di::sync::MemoryOrder::Release);
        if (current_task) {
            current_task->enable_preemption();
        }
    }

private:
    di::sync::Atomic<bool> m_state { false };
    Task* m_task { nullptr };
};
}
