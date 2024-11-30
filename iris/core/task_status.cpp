#include <iris/core/task_status.h>

#include <iris/core/task.h>

namespace iris {
void TaskStatus::set_exited() {
    m_wait_queue.notify_all([&] {
        m_exited = true;
    });
}

auto TaskStatus::wait_until_exited() -> Expected<void> {
    return m_wait_queue.wait([&] {
        return m_exited;
    });
}
}
