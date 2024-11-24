#include <iris/core/task_status.h>

#include <iris/core/task.h>

namespace iris {
void TaskStatus::set_exited() {
    return m_wait_queue.notify_all([&] {
        m_exited = true;
    });
}

Expected<void> TaskStatus::wait_until_exited() {
    return m_wait_queue.wait([&] {
        return m_exited;
    });
}
}
