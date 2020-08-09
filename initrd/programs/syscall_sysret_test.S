#ifdef __linux__
#define WRITE_NUMBER 1
#define EXIT_NUMBER 60
#else
#define WRITE_NUMBER 6
#define EXIT_NUMBER 1
#endif

.global main
.type main, @function
.section .text
main:
    mov $8, %rdi
    call strerror
    push $0
    push %rax

    mov %rax, %rdi
    call strlen
    mov %rax, %rdx
    pop %rsi

    mov $1, %rdi
    mov $WRITE_NUMBER, %rax
    syscall

    mov $EXIT_NUMBER, %rax
    mov $42, %rdi
    syscall
l:
    ud2
    jmp l

.size main, . - main
