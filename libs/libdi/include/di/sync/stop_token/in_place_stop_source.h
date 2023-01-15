#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/sync/atomic.h>
#include <di/sync/stop_token/forward_declaration.h>
#include <di/sync/stop_token/in_place_stop_callback_base.h>
#include <di/sync/synchronized.h>

namespace di::sync {
class InPlaceStopSource {
private:
    template<typename>
    friend class InPlaceStopCallback;

    friend class detail::InPlaceStopCallbackBase;

    constexpr static u8 stop_flag = 1;
    constexpr static u8 locked_flag = 2;

public:
    InPlaceStopSource() {}

    InPlaceStopSource(InPlaceStopSource&&) = delete;

    ~InPlaceStopSource() { DI_ASSERT(m_callbacks.empty()); }

    [[nodiscard]] InPlaceStopToken get_stop_token() const;
    [[nodiscard]] bool stop_requested() const { return m_state.load(MemoryOrder::Acquire) & stop_flag; }

    bool request_stop() {
        if (!lock_unless_stopped(true)) {
            // Already stopped, return false.
            return false;
        }

        // With the lock now aquired, iterate through each stop callback.
        while (!m_callbacks.empty()) {
            auto& callback = *m_callbacks.front();

            // Mark the callback as being executed, with relaxed memory order
            // since this is synchronized by the spin lock.
            callback.m_going_to_execute.store(true, MemoryOrder::Relaxed);

            // Unlock the list, allowing callback destructors the ability to
            // lock the list and remove themselves.
            unlock(true);

            // Execute the callback.
            callback.execute();

            // Mark the callback as already done.
            callback.m_already_executed.store(true, MemoryOrder::Release);

            // Reaquire the lock.
            lock(true);

            // Remove the first callback in the list. This must be the one
            // that was just executed, because callbacks won't remove themselves
            // once we set the m_going_to_execute flag.
            m_callbacks.pop_front();
        }

        unlock(true);
        return true;
    }

private:
    bool try_add_callback(detail::InPlaceStopCallbackBase* callback) const {
        if (!lock_unless_stopped(false)) {
            return false;
        }

        m_callbacks.push_front(*callback);

        unlock(false);
        return true;
    }

    void remove_callback(detail::InPlaceStopCallbackBase* callback) const {
        // Simple case: no stop request has happened.
        if (lock_unless_stopped(false)) {
            m_callbacks.erase(*callback);

            unlock(false);
            return;
        }

        // If a stop request occurred, synchronize on the spin lock.
        lock(true);

        bool going_to_be_executed = callback->m_going_to_execute.load(MemoryOrder::Relaxed);

        // If we're not going to be executed, just remove immediately.
        if (!going_to_be_executed) {
            m_callbacks.erase(*callback);
        }

        // Now unlock the spin lock.
        unlock(true);

        // If we were being executed, wait for that to complete before finishing.
        // FIXME: handle the case where the thread which called this destructor is
        //        the same as the thread that called request_stop(), by not spin waiting
        //        here, and notifying the execute loop that this object no longer exists.
        if (going_to_be_executed) {
            while (!callback->m_already_executed.load(MemoryOrder::Acquire))
                ;
        }
    }

    bool lock_unless_stopped(bool set_stop) const {
        u8 flags = set_stop ? (stop_flag | locked_flag) : locked_flag;

        u8 expected = m_state.load(MemoryOrder::Relaxed);
        for (;;) {
            // If already locked, return false.
            if (expected & stop_flag) {
                return false;
            }

            if (m_state.compare_exchange_weak(expected, flags, MemoryOrder::AcquireRelease, MemoryOrder::Relaxed)) {
                // Lock aquired, return true.
                return true;
            }
        }
    }

    void lock(bool set_stop) const {
        u8 flags = set_stop ? (stop_flag | locked_flag) : locked_flag;
        while (!m_state.exchange(flags, MemoryOrder::Acquire))
            ;
    }

    void unlock(bool set_stop) const {
        u8 flags = set_stop ? stop_flag : 0;
        m_state.store(flags, MemoryOrder::Release);
    }

    mutable container::IntrusiveList<detail::InPlaceStopCallbackBase> m_callbacks;
    mutable Atomic<u8> m_state;
};
}