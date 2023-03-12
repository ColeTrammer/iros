#pragma once

#include <iris/core/task.h>

namespace iris {
class WaitQueue {
public:
    /// @brief Notify exactly one waiter that an event occurred.
    ///
    /// @param action Callback which is executed with the WaitQueue's lock held.
    void notify_one(di::FunctionRef<void()> action);

    /// @brief Notify all waiters that an event occurred.
    ///
    /// @param action Callback which is executed with the WaitQueue's lock held.
    void notify_all(di::FunctionRef<void()> action);

    /// @brief Wait for an event to occur.
    ///
    /// This will block the currently executing task until predicate is satisfied.
    ///
    /// @param predicate Predicate which will met if this function returns successfully.
    ///
    /// @return Returns an error if the current task was interrupted by userspace, and otherwise returns success once
    ///         an event has occurred.
    Expected<void> wait(di::FunctionRef<bool()> predicate);

private:
    struct WaitQueueEntry : di::IntrusiveForwardListNode<> {
        explicit WaitQueueEntry(di::Function<void()> notify_) : notify(di::move(notify_)) {}

        di::Function<void()> notify;
    };

    di::Synchronized<di::Queue<WaitQueueEntry, di::IntrusiveForwardList<WaitQueueEntry>>> m_queue;
};
}
