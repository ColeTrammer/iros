#include "iris/core/task_namespace.h"

#include "iris/core/print.h"

namespace iris {
auto LockedTaskNamespace::allocate_task_id() -> Expected<TaskId> {
    // FIXME: use a real allocation strategy.
    // FIXME: check for overflow/maximum number of tasks taken.
    return m_next_id++;
}

auto LockedTaskNamespace::register_task(Task& task) -> Expected<void> {
    // FIXME: propogate allocation failure when di::TreeMap supports it.
    TRY(m_task_id_map.try_emplace(task.id(), task.arc_from_this()));
    return {};
}

void LockedTaskNamespace::unregister_task(Task& task) {
    m_task_id_map.erase(task.id());

    // FIXME: unallocate the task id.
}

auto LockedTaskNamespace::find_task(TaskId id) const -> Expected<di::Arc<Task>> {
    auto result = m_task_id_map.at(id);
    if (!result) {
        return di::Unexpected(Error::NoSuchProcess);
    }
    return *result;
}
}
