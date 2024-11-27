#pragma once

#include <di/vocab/pointer/prelude.h>
#include <iris/core/wait_queue.h>

namespace iris {
class TaskStatus : public di::IntrusiveRefCount<TaskStatus> {
public:
    void set_exited();

    auto wait_until_exited() -> Expected<void>;

private:
    WaitQueue m_wait_queue;
    bool m_exited { false };
};
}
