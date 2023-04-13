#pragma once

#include <iris/core/task.h>

namespace iris {
class Scheduler {
public:
    void schedule_task(Task&);

    [[noreturn]] void start();
    [[noreturn]] void start_on_ap();
    void yield();

    [[noreturn]] void save_state_and_run_next(arch::TaskState* state);
    [[noreturn]] void exit_current_task();

    Task& current_task() const { return *m_current_task; }
    Task* current_task_null_if_during_boot() const { return m_current_task; }
    mm::AddressSpace& current_address_space();

    /// @brief Block the currently running task on this scheduler.
    ///
    /// @param before_yielding Callback which is executed after updating the current task's state.
    ///
    /// @return Returns an error if the current task was interrupted by userspace, and otherwise returns success
    ///         once the task is unblocked.
    Expected<void> block_current_task(di::FunctionRef<void()> before_yielding);

    void setup_idle_task();

private:
    [[noreturn]] void run_next();

    Task* m_current_task { nullptr };
    di::Queue<Task, di::IntrusiveList<Task>> m_run_queue;
    di::Arc<Task> m_idle_task;
};
}
