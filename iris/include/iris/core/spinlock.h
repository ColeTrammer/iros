#pragma once

#include "di/sync/atomic.h"
#include "di/util/immovable.h"
#include "iris/core/interrupt_disabler.h"

namespace iris {
class Spinlock : di::util::Immovable {
public:
    Spinlock() = default;

    void lock();
    auto try_lock() -> bool;
    void unlock();

private:
    di::sync::Atomic<bool> m_state { false };
    bool m_interrupts_were_disabled { false };
};
}
