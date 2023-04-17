#pragma once

#include <di/container/tree/prelude.h>
#include <di/sync/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/core/error.h>
#include <iris/core/task.h>

namespace iris {
class LockedTaskNamespace {
public:
    Expected<TaskId> allocate_task_id();

    Expected<void> register_task(Task&);
    void unregister_task(Task&);

    Expected<di::Arc<Task>> find_task(TaskId id) const;

private:
    TaskId m_next_id { 0 };
    di::TreeMap<TaskId, di::Arc<Task>> m_task_id_map;
};

class TaskNamespace
    : public di::IntrusiveRefCount<TaskNamespace>
    , public di::Synchronized<LockedTaskNamespace, InterruptibleSpinlock> {};
}
