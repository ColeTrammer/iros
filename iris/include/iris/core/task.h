#pragma once

#include <di/prelude.h>
#include <iris/core/config.h>
#include <iris/fs/file.h>
#include <iris/mm/address_space.h>

// clang-format off
#include IRIS_ARCH_INCLUDE(core/task.h)
// clang-format on

namespace iris {
struct TaskIdTag : di::meta::TypeConstant<i32> {};

using TaskId = di::StrongInt<TaskIdTag>;

class TaskNamespace;

class Task
    : public di::IntrusiveListElement<>
    , public di::IntrusiveRefCount<Task> {
public:
    explicit Task(mm::VirtualAddress entry, mm::VirtualAddress stack, bool userspace,
                  di::Arc<mm::AddressSpace> address_space, di::Arc<TaskNamespace> task_namespace, TaskId task_id,
                  di::Arc<FileTable> file_table);

    ~Task();

    [[noreturn]] void context_switch_to() {
        m_address_space->load();
        m_task_state.context_switch_to();
    }

    arch::TaskState const& task_state() const { return m_task_state; }
    void set_task_state(arch::TaskState const& state) { m_task_state = state; }

    TaskId id() const { return m_id; }
    mm::AddressSpace& address_space() { return *m_address_space; }
    TaskNamespace& task_namespace() const { return *m_task_namespace; }
    FileTable& file_table() const { return *m_file_table; }

    void set_instruction_pointer(mm::VirtualAddress address);

private:
    arch::TaskState m_task_state;
    di::Arc<mm::AddressSpace> m_address_space;
    di::Arc<TaskNamespace> m_task_namespace;
    di::Arc<FileTable> m_file_table;
    TaskId m_id;
};

Expected<di::Arc<Task>> create_kernel_task(TaskNamespace&, void (*entry)());
Expected<di::Arc<Task>> create_user_task(TaskNamespace&, di::Arc<FileTable> file_table);
Expected<void> load_executable(Task&, di::PathView path);
}
