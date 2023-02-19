#pragma once

#include <di/prelude.h>
#include <iris/mm/address_space.h>

namespace iris {
namespace arch {
    // This is x86_64 specific.
    struct TaskState {
        explicit TaskState(u64 entry, u64 stack, bool userspace);

        // Function to perform a context switch.
        [[noreturn]] void context_switch_to();

        // These are the main x86_64 registers.
        u64 r15 { 0 };
        u64 r14 { 0 };
        u64 r13 { 0 };
        u64 r12 { 0 };
        u64 r11 { 0 };
        u64 r10 { 0 };
        u64 r9 { 0 };
        u64 r8 { 0 };
        u64 rbp { 0 };
        u64 rdi { 0 };
        u64 rsi { 0 };
        u64 rdx { 0 };
        u64 rcx { 0 };
        u64 rbx { 0 };
        u64 rax { 0 };

        // This is the task state present on the stack after
        // an interrupt occurs.
        u64 rip { 0 };
        u64 cs { 0 };
        u64 rflags { 0 };
        u64 rsp { 0 };
        u64 ss { 0 };
    };
}

class Task : public di::IntrusiveListElement<> {
public:
    explicit Task(mm::VirtualAddress entry, mm::VirtualAddress stack, bool userspace,
                  di::Arc<mm::AddressSpace> address_space)
        : m_task_state(entry.raw_address(), stack.raw_address(), userspace), m_address_space(di::move(address_space)) {}

    [[noreturn]] void context_switch_to() {
        // m_address_space->load();
        m_task_state.context_switch_to();
    }

    void set_task_state(arch::TaskState const& state) { m_task_state = state; }

private:
    arch::TaskState m_task_state;
    di::Arc<mm::AddressSpace> m_address_space;
};

Expected<di::Box<Task>> create_kernel_task(void (*entry)());
Expected<di::Box<Task>> create_user_task(di::PathView path);
}