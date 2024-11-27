#pragma once

#include <di/sync/atomic.h>
#include <di/util/immovable.h>
#include <iris/core/preemption.h>

namespace iris {
class Task;

class InterruptibleSpinlock : di::util::Immovable {
public:
    InterruptibleSpinlock() = default;

    void lock();
    auto try_lock() -> bool;
    void unlock();

private:
    di::sync::Atomic<bool> m_state { false };
    Task* m_task { nullptr };
};
}
