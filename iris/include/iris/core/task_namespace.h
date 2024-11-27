#pragma once

#include <di/container/tree/prelude.h>
#include <di/sync/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/core/error.h>
#include <iris/core/task.h>

namespace iris {
class LockedTaskNamespace {
public:
    auto allocate_task_id() -> Expected<TaskId>;

    auto register_task(Task&) -> Expected<void>;
    void unregister_task(Task&);

    auto find_task(TaskId id) const -> Expected<di::Arc<Task>>;

private:
    TaskId m_next_id { 0 };
    di::TreeMap<TaskId, di::Arc<Task>> m_task_id_map;
};

class TaskNamespace
    : public di::IntrusiveRefCount<TaskNamespace>
    , public di::Synchronized<LockedTaskNamespace, InterruptibleSpinlock> {};
}
