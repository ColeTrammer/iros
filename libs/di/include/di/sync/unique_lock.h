#pragma once

#include "di/assert/assert_bool.h"
#include "di/sync/concepts/lock.h"
#include "di/util/exchange.h"
#include "di/util/swap.h"

namespace di::sync {
struct DeferLock {};
struct TryToLock {};
struct AdoptLock {};

constexpr inline auto defer_lock = DeferLock {};
constexpr inline auto try_to_lock = TryToLock {};
constexpr inline auto adopt_lock = AdoptLock {};

template<concepts::Lock Lock>
class UniqueLock {
public:
    UniqueLock() = default;
    constexpr UniqueLock(UniqueLock&& other)
        : m_mutex(di::exchange(other.m_mutex, nullptr)), m_owned(di::exchange(other.m_owned, false)) {}

    constexpr explicit UniqueLock(Lock& mutex) : m_mutex(&mutex) { lock(); }

    constexpr UniqueLock(Lock& mutex, DeferLock) : m_mutex(&mutex) {}
    constexpr UniqueLock(Lock& mutex, TryToLock) : m_mutex(&mutex) { try_lock(); }
    constexpr UniqueLock(Lock& mutex, AdoptLock) : m_mutex(&mutex), m_owned(true) {}

    constexpr ~UniqueLock() {
        if (m_owned && m_mutex) {
            unlock();
        }
    }

    constexpr auto operator=(UniqueLock&& other) -> UniqueLock& {
        if (m_owned && m_mutex) {
            unlock();
        }
        m_mutex = di::exchange(other.m_mutex, nullptr);
        m_owned = di::exchange(other.m_owned, false);
        return *this;
    }

    constexpr void lock() {
        DI_ASSERT(m_mutex);
        DI_ASSERT(!m_owned);
        m_mutex->lock();
        m_owned = true;
    }

    constexpr auto try_lock() -> bool {
        DI_ASSERT(m_mutex);
        DI_ASSERT(!m_owned);
        m_owned = m_mutex->try_lock();
        return m_owned;
    }

    constexpr void unlock() {
        DI_ASSERT(m_mutex);
        DI_ASSERT(m_owned);
        m_mutex->unlock();
    }

    constexpr void swap(UniqueLock& other) {
        di::swap(m_mutex, other.m_mutex);
        di::swap(m_owned, other.m_owned);
    }

    constexpr auto release() -> Lock* {
        auto result = di::exchange(m_mutex, nullptr);
        m_owned = false;
        return result;
    }

    constexpr auto mutex() const -> Lock* { return m_mutex; }

    constexpr auto owns_lock() const -> bool { return m_owned; }
    constexpr explicit operator bool() const { return owns_lock(); }

private:
    Lock* m_mutex { nullptr };
    bool m_owned { false };
};
}

namespace di {
using sync::AdoptLock;
using sync::DeferLock;
using sync::TryToLock;
using sync::UniqueLock;

using sync::adopt_lock;
using sync::defer_lock;
using sync::try_to_lock;
}
