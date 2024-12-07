#pragma once

#include "di/sync/concepts/lock.h"
#include "di/vocab/tuple/prelude.h"

namespace di::sync {
template<concepts::Lock... Locks>
class ScopedLock {
public:
    constexpr explicit ScopedLock(Locks&... locks) : m_locks(locks...) {
        // FIXME: this should use a deadlock avoidance algorithm (lock in order of lock addresses).
        tuple_for_each(
            [](concepts::Lock auto& lock) {
                lock.lock();
            },
            m_locks);
    }

    ScopedLock(ScopedLock const&) = delete;
    auto operator=(ScopedLock const&) -> ScopedLock& = delete;

    constexpr ~ScopedLock() {
        tuple_for_each(
            [](concepts::Lock auto& lock) {
                lock.unlock();
            },
            m_locks);
    }

private:
    Tuple<Locks&...> m_locks;
};
}

namespace di {
using sync::ScopedLock;
}
