#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/platform/prelude.h>
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

        // Remember the thread id which requested the stop.
        m_stopper_thread = get_current_thread_id();

        // With the lock now aquired, iterate through each stop callback.
        while (!m_callbacks.empty()) {
            auto& callback = *m_callbacks.front();

            // Mark the callback as being executed, with relaxed memory order
            // since this is synchronized by the spin lock.
            bool did_destroy_itself = false;
            callback.m_did_destruct_in_same_thread.store(util::addressof(did_destroy_itself), MemoryOrder::Relaxed);

            // Remove the current callback from the list.
            m_callbacks.pop_front();

            // Unlock the list, allowing callback destructors the ability to
            // lock the list and remove themselves.
            unlock(true);

            // Execute the callback.
            callback.execute();

            // Mark the callback as already done, if the object still exists.
            if (!did_destroy_itself) {
                callback.m_already_executed.store(true, MemoryOrder::Release);
            }

            // Reaquire the lock.
            lock(true);
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

        auto stopper_thread = m_stopper_thread;
        auto* did_destruct_in_same_thread = callback->m_did_destruct_in_same_thread.load(MemoryOrder::Relaxed);
        bool going_to_be_executed = !!did_destruct_in_same_thread;

        // Remove ourselves from the list with the lock held.
        if (!going_to_be_executed) {
            m_callbacks.erase(*callback);
        }

        // Now unlock the spin lock.
        unlock(true);

        if (going_to_be_executed) {
            // If we are being executed by the current thread, notify the callback runner this object
            // has been destroyed. This is not synchronized because we must running on the same thread.
            if (stopper_thread == get_current_thread_id()) {
                *did_destruct_in_same_thread = true;
            } else {
                // Otherwise, wait for the callback's execution to complete before finishing.
                while (!callback->m_already_executed.load(MemoryOrder::Acquire))
                    ;
            }
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
    mutable Atomic<u8> m_state { 0 };
    ThreadId m_stopper_thread {};
};
}
