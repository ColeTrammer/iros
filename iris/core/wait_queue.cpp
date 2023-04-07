#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/core/task.h>
#include <iris/core/wait_queue.h>

namespace iris {
void WaitQueue::notify_one(di::FunctionRef<void()> action) {
    m_queue.with_lock([&](auto& queue) {
        action();
        if (auto entry = queue.pop()) {
            entry->notify();
        }
    });
}

void WaitQueue::notify_all(di::FunctionRef<void()> action) {
    m_queue.with_lock([&](auto& queue) {
        action();
        while (auto entry = queue.pop()) {
            entry->notify();
        }
    });
}

Expected<void> WaitQueue::wait(di::FunctionRef<bool()> predicate) {
    auto& lock = m_queue.get_lock();
    for (;;) {
        // Acquire the queue's lock.
        lock.lock();

        // SAFETY: This is safe, because acquiring the lock disables interrupts.
        auto& scheduler = current_processor_unsafe().scheduler();
        auto& current_task = scheduler.current_task();

        // If the predicate is now satisified, stop looping.
        if (predicate()) {
            lock.unlock();
            return {};
        }

        // Setup a wait queue entry which will unblock this task.
        auto entry = WaitQueueEntry { [&] {
            // FIXME: dynamically choose a scheduler on SMP systems.
            scheduler.schedule_task(current_task);
        } };
        m_queue.get_assuming_no_concurrent_accesses().push(entry);

        // Now, we must block the current task.
        TRY(scheduler.block_current_task([&] {
            lock.unlock();
        }));
    }
}
}
