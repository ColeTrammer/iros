#pragma once

#include <di/prelude.h>

#include <iris/core/error.h>
#include <iris/core/task.h>

namespace iris {
class TaskNamespace : public di::IntrusiveRefCount<TaskNamespace> {
public:
    Expected<TaskId> allocate_task_id();

    Expected<void> register_task(Task&);
    void unregister_task(Task&);

    Expected<Task&> find_task(TaskId id) const;

private:
    TaskId m_next_id { 0 };
    di::TreeMap<TaskId, di::Arc<Task>> m_task_id_map;
};
}
