#include <setjmp.h>

extern "C" {
[[gnu::naked]] int setjmp([[maybe_unused]] jmp_buf env) {
    asm volatile(
        // Save the SYS-V ABI callee-saved registers.
        "movq %rbp, 0(%rdi)\n"
        "movq %rbx, 8(%rdi)\n"
        "movq %r12, 16(%rdi)\n"
        "movq %r13, 24(%rdi)\n"
        "movq %r14, 32(%rdi)\n"
        "movq %r15, 40(%rdi)\n"

        // Save the stack pointer to return to (add 8 to the stack pointer to skip the return address).
        "leaq 8(%rsp), %rax\n"
        "movq %rax, 48(%rdi)\n"

        // Save the instruction pointer to return to (read from the return address on the stack).
        "movq (%rsp), %rax\n"
        "movq %rax, 56(%rdi)\n"

        // Return 0.
        "xorl %eax, %eax\n"
        "ret\n");
}
}
