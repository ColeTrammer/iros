#pragma once

#include <di/prelude.h>
#include <iris/core/preemption.h>

namespace iris {
class Task;

class InterruptibleSpinlock : di::util::Immovable {
public:
    InterruptibleSpinlock() = default;

    void lock();
    bool try_lock();
    void unlock();

private:
    di::sync::Atomic<bool> m_state { false };
    Task* m_task { nullptr };
};
}
