#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/sync/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/pointer/prelude.h>
#include <iris/core/config.h>
#include <iris/core/task_arguments.h>
#include <iris/core/task_status.h>
#include <iris/fs/file.h>
#include <iris/fs/tnode.h>
#include <iris/mm/address_space.h>

#include IRIS_ARCH_INCLUDE(core/task.h)

namespace iris {
struct TaskIdTag : di::meta::TypeConstant<i32> {};

using TaskId = di::StrongInt<TaskIdTag>;

class TaskNamespace;

struct TaskFinalizationRequest {
    di::Arc<mm::AddressSpace> address_space;
    mm::VirtualAddress kernel_stack;
};

class Task
    : public di::IntrusiveListNode<>
    , public di::IntrusiveRefCount<Task> {
public:
    explicit Task(bool userspace, di::Arc<mm::AddressSpace> address_space, di::Arc<TaskNamespace> task_namespace,
                  TaskId task_id, FileTable file_table, di::Arc<TaskStatus> task_status);

    ~Task();

    [[noreturn]] void context_switch_to() {
        // NOTE: Disable interrupts at this point because setting the userspace thread pointer may make accessing the
        // current processor impossible. If we receive an interrupt after that point, we will crash. NMI's have to take
        // special care to handle this scenario. Additionally, we want to avoid any possible race conditions while
        // context switching, as these functions may need to access the current processor.
        auto guard = InterruptDisabler {};
        m_address_space->load();
        m_fpu_state.load();
        if (m_kernel_stack.raw_value() != 0) {
            arch::load_kernel_stack(m_kernel_stack + 0x2000zu);
        }
        arch::load_userspace_thread_pointer(userspace_thread_pointer(), m_task_state);

        m_task_state.context_switch_to();
    }

    arch::TaskState const& task_state() const { return m_task_state; }
    void set_task_state(arch::TaskState const& state) { m_task_state = state; }

    di::Arc<TaskArguments> task_arguments() const { return m_task_arguments; }
    void set_task_arguments(di::Arc<TaskArguments> task_arguments) { m_task_arguments = di::move(task_arguments); }

    arch::FpuState& fpu_state() { return m_fpu_state; }
    arch::FpuState const& fpu_state() const { return m_fpu_state; }

    TaskId id() const { return m_id; }

    mm::AddressSpace& address_space() { return *m_address_space; }
    void set_address_space(di::Arc<mm::AddressSpace> address_space) { m_address_space = di::move(address_space); }

    TaskNamespace& task_namespace() const { return *m_task_namespace; }
    FileTable& file_table() { return m_file_table; }

    void set_instruction_pointer(mm::VirtualAddress address) {
        m_task_state.set_instruction_pointer(address.raw_value());
    }
    void set_stack_pointer(mm::VirtualAddress address) { m_task_state.set_stack_pointer(address.raw_value()); }
    void set_argument1(uptr value) { m_task_state.set_argument1(value); }
    void set_argument2(uptr value) { m_task_state.set_argument2(value); }
    void set_argument3(uptr value) { m_task_state.set_argument3(value); }
    void set_argument4(uptr value) { m_task_state.set_argument4(value); }

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

    uptr userspace_thread_pointer() const { return m_userspace_thread_pointer; }
    void set_userspace_thread_pointer(uptr userspace_thread_pointer) {
        m_userspace_thread_pointer = userspace_thread_pointer;
    }

    di::Arc<TNode> root_tnode() const { return m_root_tnode; }
    void set_root_tnode(di::Arc<TNode> root_tnode) { m_root_tnode = di::move(root_tnode); }

    di::Arc<TNode> cwd_tnode() const { return m_cwd_tnode; }
    void set_cwd_tnode(di::Arc<TNode> cwd_tnode) { m_cwd_tnode = di::move(cwd_tnode); }

private:
    arch::TaskState m_task_state;
    arch::FpuState m_fpu_state;
    di::Arc<mm::AddressSpace> m_address_space;
    di::Arc<TaskNamespace> m_task_namespace;
    di::Arc<TNode> m_root_tnode;
    di::Arc<TNode> m_cwd_tnode;
    di::Atomic<i32> m_preemption_disabled_count;
    di::Atomic<bool> m_should_be_preempted { false };
    di::Arc<TaskStatus> m_task_status;
    di::Arc<TaskArguments> m_task_arguments;
    di::Atomic<bool> m_waiting { false };
    mm::VirtualAddress m_kernel_stack { 0 };
    uptr m_userspace_thread_pointer { 0 };
    FileTable m_file_table;
    TaskId m_id;
};

Expected<di::Arc<Task>> create_kernel_task(TaskNamespace&, void (*entry)());
Expected<di::Arc<Task>> create_user_task(TaskNamespace&, di::Arc<TNode> root_tnode, di::Arc<TNode> cwd_tnode, FileTable,
                                         di::Arc<mm::AddressSpace>);
Expected<void> load_executable(Task&, di::PathView path);

Expected<u64> do_syscall(Task& current_task, arch::TaskState& task_state);
}
