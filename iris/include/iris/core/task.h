#pragma once

#include <di/prelude.h>
#include <iris/core/config.h>
#include <iris/mm/address_space.h>

// clang-format off
#include IRIS_ARCH_INCLUDE(core/task.h)
// clang-format on

namespace iris {
class Task
    : public di::IntrusiveListElement<>
    , public di::IntrusiveRefCount<Task> {
public:
    explicit Task(mm::VirtualAddress entry, mm::VirtualAddress stack, bool userspace,
                  di::Arc<mm::AddressSpace> address_space)
        : m_task_state(entry.raw_value(), stack.raw_value(), userspace), m_address_space(di::move(address_space)) {
        // Explicitly leak one reference to the task, which will be dropped when exit_task() is called.
        (void) arc_from_this().release();
    }

    [[noreturn]] void context_switch_to() {
        m_address_space->load();
        m_task_state.context_switch_to();
    }

    arch::TaskState const& task_state() const { return m_task_state; }
    void set_task_state(arch::TaskState const& state) { m_task_state = state; }

private:
    arch::TaskState m_task_state;
    di::Arc<mm::AddressSpace> m_address_space;
};

Expected<di::Arc<Task>> create_kernel_task(void (*entry)());
Expected<di::Arc<Task>> create_user_task(di::PathView path);
}
