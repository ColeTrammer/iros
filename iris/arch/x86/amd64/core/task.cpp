#include <iris/core/task.h>

namespace iris::arch {
TaskState::TaskState(u64 entry, u64 stack, bool userspace) : rip(entry), rsp(stack) {
    if (userspace) {
        ss = 8 * 8 + 3;
        cs = 7 * 8 + 3;
    } else {
        ss = 0 * 8 + 0;
        cs = 5 * 8 + 0;
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
}

namespace iris {
void Task::set_instruction_pointer(mm::VirtualAddress address) {
    m_task_state.rip = address.raw_value();
}
}
