#pragma once

#include <iris/core/task.h>

namespace iris {
class Scheduler {
public:
    void schedule_task(Task&);

    [[noreturn]] void start();
    void yield();

    [[noreturn]] void save_state_and_run_next(arch::TaskState* state);

private:
    [[noreturn]] void run_next();

    Task* m_current_task { nullptr };
    di::Queue<Task, di::IntrusiveList<Task>> m_run_queue;
};
}