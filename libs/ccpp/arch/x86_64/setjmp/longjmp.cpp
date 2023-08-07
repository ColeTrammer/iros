#include <setjmp.h>

extern "C" {
[[gnu::naked]] void longjmp([[maybe_unused]] jmp_buf env, [[maybe_unused]] int val) {
    asm volatile(
        // Load the SYS-V ABI callee-saved registers.
        "movq 0(%rdi), %rbp\n"
        "movq 8(%rdi), %rbx\n"
        "movq 16(%rdi), %r12\n"
        "movq 24(%rdi), %r13\n"
        "movq 32(%rdi), %r14\n"
        "movq 40(%rdi), %r15\n"

        // Load the stack pointer.
        "mov 48(%rdi), %rsp\n"

        // Check if val is 0.
        "testl %esi, %esi\n"
        "jnz __skip_fixup_val\n"

        // If val is 0, set it to 1.
        "movl $1, %esi\n"

        // Set the return value to val.
        "__skip_fixup_val:\n"
        "movl %esi, %eax\n"

        // Jump to the saved instruction pointer.
        "jmp *56(%rdi)\n");
}
}
