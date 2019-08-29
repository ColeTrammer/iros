.section .text
.global run_process
run_process:
    pushq $0x23
    pushq %rsi
    pushfq
    pushq $(0x18 | 0x03)
    pushq %rdi
    iretq