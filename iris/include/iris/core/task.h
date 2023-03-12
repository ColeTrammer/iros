#pragma once

#include <di/prelude.h>
#include <iris/core/config.h>
#include <iris/core/task_status.h>
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
    : public di::IntrusiveListNode<>
    , public di::IntrusiveRefCount<Task> {
public:
    explicit Task(mm::VirtualAddress entry, mm::VirtualAddress stack, bool userspace,
                  di::Arc<mm::AddressSpace> address_space, di::Arc<TaskNamespace> task_namespace, TaskId task_id,
                  FileTable file_table, di::Arc<TaskStatus> task_status);

    ~Task();

    [[noreturn]] void context_switch_to() {
        m_address_space->load();
        m_fpu_state.load();
        if (m_kernel_stack.raw_value() != 0) {
            arch::load_kernel_stack(m_kernel_stack + 0x2000);
        }
        m_task_state.context_switch_to();
    }

    arch::TaskState const& task_state() const { return m_task_state; }
    void set_task_state(arch::TaskState const& state) { m_task_state = state; }

    arch::FpuState& fpu_state() { return m_fpu_state; }
    arch::FpuState const& fpu_state() const { return m_fpu_state; }

    TaskId id() const { return m_id; }
    mm::AddressSpace& address_space() { return *m_address_space; }
    TaskNamespace& task_namespace() const { return *m_task_namespace; }
    FileTable& file_table() { return m_file_table; }

    void set_instruction_pointer(mm::VirtualAddress address);

    bool preemption_disabled() const { return m_preemption_disabled_count.load(di::MemoryOrder::Relaxed) > 0; }
    void disable_preemption() { m_preemption_disabled_count.fetch_add(1, di::MemoryOrder::Relaxed); }
    void enable_preemption();
    void set_should_be_preempted() { m_should_be_preempted.store(true, di::MemoryOrder::Relaxed); }

    di::Arc<TaskStatus> task_status() const { return m_task_status; }

    bool waiting() const { return m_waiting.load(di::MemoryOrder::Relaxed); }
    void set_waiting() { return m_waiting.store(true, di::MemoryOrder::Relaxed); }
    void set_runnable() { return m_waiting.store(false, di::MemoryOrder::Relaxed); }

    mm::VirtualAddress kernel_stack() const { return m_kernel_stack; }
    void set_kernel_stack(mm::VirtualAddress kernel_stack) { m_kernel_stack = kernel_stack; }

private:
    arch::TaskState m_task_state;
    arch::FpuState m_fpu_state;
    di::Arc<mm::AddressSpace> m_address_space;
    di::Arc<TaskNamespace> m_task_namespace;
    di::Atomic<i32> m_preemption_disabled_count;
    di::Atomic<bool> m_should_be_preempted { false };
    di::Arc<TaskStatus> m_task_status;
    di::Atomic<bool> m_waiting { false };
    mm::VirtualAddress m_kernel_stack { 0 };
    FileTable m_file_table;
    TaskId m_id;
};

Expected<di::Arc<Task>> create_kernel_task(TaskNamespace&, void (*entry)());
Expected<di::Arc<Task>> create_user_task(TaskNamespace&, FileTable file_table);
Expected<void> load_executable(Task&, di::PathView path);
}
