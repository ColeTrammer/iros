#include <iris/arch/x86/amd64/system_instructions.h>
#include <iris/core/global_state.h>
#include <iris/core/interrupt_disabler.h>
#include <iris/core/print.h>
#include <iris/core/task.h>

namespace iris::arch {
TaskState::TaskState(bool userspace) {
    if (userspace) {
        ss = 8 * 8 + 3;
        cs = 7 * 8 + 3;
        rflags = interrupt_enable_flag | 2;
    } else {
        ss = 0 * 8 + 0;
        cs = 5 * 8 + 0;
        rflags = interrupt_enable_flag | 2;
    }
}

[[gnu::naked]] void TaskState::context_switch_to() {
    // The task state is passed in the %rdi register.
    // Simply load all registers from the task state pointer,
    // making sure to load %rdi last so that the pointer is always
    // accessible. The actual context switch is performed using the
    // iretq instruction, based on the interrupt frame pushed onto
    // the stack.
    asm volatile("movq (%rdi), %r15\n"
                 "movq 8(%rdi), %r14\n"
                 "movq 16(%rdi), %r13\n"
                 "movq 24(%rdi), %r12\n"
                 "movq 32(%rdi), %r11\n"
                 "movq 40(%rdi), %r10\n"
                 "movq 48(%rdi), %r9\n"
                 "movq 56(%rdi), %r8\n"
                 "movq 64(%rdi), %rbp\n"

                 "movq 80(%rdi), %rsi\n"
                 "movq 88(%rdi), %rdx\n"
                 "movq 96(%rdi), %rcx\n"
                 "movq 104(%rdi), %rbx\n"
                 "movq 112(%rdi), %rax\n"

                 "pushq 152(%rdi)\n"
                 "pushq 144(%rdi)\n"
                 "pushq 136(%rdi)\n"
                 "pushq 128(%rdi)\n"
                 "pushq 120(%rdi)\n"

                 "movq 72(%rdi), %rdi\n"

                 "iretq\n");
}

FpuState::~FpuState() {
    if (fpu_state) {
        ::operator delete(fpu_state, global_state().processor_info.fpu_max_state_size, std::align_val_t { 64 });
    }
}

Expected<void> FpuState::setup_fpu_state() {
    fpu_state = TRY(allocate_fpu_state());

    auto* clean_fpu_state = global_state().initial_fpu_state.fpu_state;
    auto fpu_size = global_state().processor_info.fpu_max_state_size;
    di::copy_n(clean_fpu_state, fpu_size, fpu_state);

    return {};
}

Expected<void> FpuState::setup_initial_fpu_state() {
    global_state_in_boot().boot_processor.arch_processor().setup_fpu_support_for_processor();

    fpu_state = TRY(allocate_fpu_state());
    save();

    return {};
}

Expected<di::Byte*> FpuState::allocate_fpu_state() {
    auto fpu_size = global_state().processor_info.fpu_max_state_size;

    // NOTE: 64 byte alignment is required when using SSE extensions.
    auto result = ::operator new(fpu_size, std::align_val_t { 64 }, std::nothrow);
    if (!result) {
        return di::Unexpected(Error::NotEnoughMemory);
    }
    return static_cast<di::Byte*>(result);
}

void FpuState::load() {
    if (fpu_state) {
        if (global_state().processor_info.has_xsave()) {
            // NOTE: modern processors may have optimized versions of these instructions for use in the kernel.
            x86::amd64::xrstor(fpu_state);
        } else {
            x86::amd64::fxrstor(fpu_state);
        }
    }
}

void FpuState::save() {
    if (fpu_state) {
        if (global_state().processor_info.has_xsave()) {
            // NOTE: modern processors may have optimized versions of these instructions for use in the kernel. Such as:
            //       xsaveopt.
            x86::amd64::xsave(fpu_state);
        } else {
            x86::amd64::fxsave(fpu_state);
        }
    }
}
}
