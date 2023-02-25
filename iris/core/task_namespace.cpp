#include <iris/core/task_namespace.h>

namespace iris {
Expected<TaskId> TaskNamespace::allocate_task_id() {
    // FIXME: use a real allocation strategy.
    // FIXME: check for overflow/maximum number of tasks taken.
    return m_next_id++;
}

Expected<void> TaskNamespace::register_task(Task& task) {
    // FIXME: propogate allocation failure when di::TreeMap supports it.
    m_task_id_map.try_emplace(task.id(), task.arc_from_this());
    return {};
}

void TaskNamespace::unregister_task(Task& task) {
    m_task_id_map.erase(task.id());

    // FIXME: unallocate the task id.
}

Expected<Task&> TaskNamespace::find_task(TaskId id) const {
    auto result = m_task_id_map.at(id);
    if (!result) {
        return di::Unexpected(Error::NoSuchProcess);
    }
    return **result;
}
}
