#include <dius/linux/io_uring_context.h>

namespace dius::linux {
IoUringContext::~IoUringContext() = default;

void IoUringContext::run() {
    for (;;) {
        // Reap any pending completions.
        while (auto cqe = m_handle.get_next_cqe()) {
            auto* as_operation = reinterpret_cast<OperationStateBase*>(cqe->user_data);
            as_operation->did_complete(cqe.data());
        }

        // Run locally available operations.
        while (!m_queue.empty()) {
            auto& item = *m_queue.pop();
            item.execute();
        }

        // If we're done, we're done.
        if (m_done.load()) {
            break;
        }

        // Wait for some event to happen.
        (void) m_handle.submit_and_wait();
    }
}
}
