#pragma once

#include <iris/core/wait_queue.h>

namespace iris {
class TaskStatus : public di::IntrusiveRefCount<TaskStatus> {
public:
    void set_exited();

    Expected<void> wait_until_exited();

private:
    WaitQueue m_wait_queue;
    bool m_exited { false };
};
}
