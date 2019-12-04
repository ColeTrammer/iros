#ifndef _ARCH_X86_64_OS_2_H
#define _ARCH_X86_64_OS_2_H 1

#define MUTEX_AQUIRE  1
#define MUTEX_RELEASE 2

#ifndef __is_kernel

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

__attribute__((always_inline)) static inline int create_task(unsigned long rip, unsigned long rsp, void *arg, unsigned long push_onto_stack,
                                                             int *tid_ptr) {
    int ret;
    asm volatile("movq $55, %%rdi\n"
                 "movq %1, %%rsi\n"
                 "movq %2, %%rdx\n"
                 "movq %3, %%rcx\n"
                 "movq %4, %%r8\n"
                 "movq %5, %%r9\n"
                 "int $0x80\n"
                 "movl %%eax, %0\n"
                 : "=r"(ret)
                 : "r"(rip), "r"(rsp), "r"(arg), "r"(push_onto_stack), "r"(tid_ptr)
                 : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "memory");
    return ret;
}

__attribute__((__noreturn__)) __attribute__((always_inline)) void exit_task(void) {
    asm volatile("movq $56, %%rdi\n"
                 "int $0x80"
                 :
                 :
                 : "rdi", "memory");
    __builtin_unreachable();
}

__attribute__((always_inline)) static inline int gettid(void) {
    int ret;
    asm volatile("movq $57, %%rdi\n"
                 "int $0x80\n"
                 "movl %%eax, %0"
                 : "=r"(ret)
                 :
                 : "rdi", "rax", "memory");
    return ret;
}

__attribute__((always_inline)) static inline int os_mutex(int *__protected, int operation, int expected, int to_place) {
    int ret;
    asm volatile("movq $58, %%rdi\n"
                 "movq %1, %%rsi\n"
                 "movl %2, %%edx\n"
                 "movl %3, %%ecx\n"
                 "movl %4, %%r8\n"
                 "int $0x80\n"
                 "movl %%eax, %0"
                 : "=r"(ret)
                 : "r"(__protected), "r"(operation), "r"(expected), "r"(to_place)
                 : "rdi", "rsi", "rdx", "rcx", "rax", "memory");
    return ret;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __is_kernel */

#endif /* _ARCH_X86_64_OS_2_H */