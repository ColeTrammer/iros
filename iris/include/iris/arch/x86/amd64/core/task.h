#pragma once

#include <di/prelude.h>

namespace iris::arch {
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