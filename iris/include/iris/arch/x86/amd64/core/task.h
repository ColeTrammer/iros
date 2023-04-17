#pragma once

#include <di/types/prelude.h>

#include <di/util/prelude.h>
#include <iris/core/error.h>
#include <iris/uapi/syscall.h>

namespace iris::arch {
// This is x86_64 specific.
struct TaskState {
    explicit TaskState(bool userspace);

    SystemCall syscall_number() const { return SystemCall(rax); }

    bool in_kernel() const { return (cs & 0x3) == 0; }

    u64 syscall_arg1() const { return rdi; }
    u64 syscall_arg2() const { return rsi; }
    u64 syscall_arg3() const { return rdx; }
    u64 syscall_arg4() const { return r10; }
    u64 syscall_arg5() const { return r8; }
    u64 syscall_arg6() const { return r9; }

    void set_syscall_return(Expected<uptr> value) {
        if (value) {
            rdx = 0;
            rax = *value;
        } else {
            rdx = di::to_underlying(value.error());
        }
    }

    void set_instruction_pointer(uptr value) { rip = value; }
    void set_stack_pointer(uptr value) { rsp = value; }
    void set_argument1(uptr value) { rdi = value; }
    void set_argument2(uptr value) { rsi = value; }
    void set_argument3(uptr value) { rdx = value; }
    void set_argument4(uptr value) { rcx = value; }

    /// Function to perform a context switch.
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

struct FpuState {
    FpuState() {}

    ~FpuState();

    /// Setup the task's FPU state. This must be called for userspace tasks.
    Expected<void> setup_fpu_state();

    /// Setup initial FPU state. This creates a clean-copy of the FPU state which is copied to newly created tasks. This
    /// also configures the processor to allow floating-point / SIMD operations.
    Expected<void> setup_initial_fpu_state();

    /// Load this task's FPU state into the registers. This only makes sense to call when IRQs are disabled, right
    /// before performing a context switch.
    void load();

    /// Save the current processor's FPU state into this task. This only makes sense to call when IRQs are disabled,
    /// when the task has just been interrupted.
    void save();

    /// This is the task's FPU state. It is null for kernel-space tasks. Since it is dynamically sized, it must be
    /// managed manually.
    byte* fpu_state { nullptr };

private:
    Expected<byte*> allocate_fpu_state();
};

void load_kernel_stack(mm::VirtualAddress base);
void load_userspace_thread_pointer(uptr userspace_thread_pointer, TaskState& task_state);
}
