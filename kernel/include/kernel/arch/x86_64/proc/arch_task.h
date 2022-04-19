#ifndef _KERNEL_ARCH_X86_64_PROC_ARCH_TASK_H
#define _KERNEL_ARCH_X86_64_PROC_ARCH_TASK_H 1

#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h>

#include <kernel/arch/x86/proc/fpu.h>
#include <kernel/mem/page.h>

#define KERNEL_STACK_SIZE PAGE_SIZE

struct cpu_state {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
} __attribute__((packed));

struct stack_state {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

struct task_state {
    struct cpu_state cpu_state;
    struct stack_state stack_state;
} __attribute__((packed));

struct arch_task {
    struct task_state task_state;
    void *user_thread_pointer;
};

struct task;

void task_setup_user_state(struct task_state *task_state);

extern void __run_task(struct arch_task *state);

static inline void task_set_ip(struct task_state *task_state, uintptr_t ip) {
    task_state->stack_state.rip = ip;
}

static inline void task_set_sp(struct task_state *task_state, uintptr_t sp) {
    task_state->stack_state.rsp = sp;
}

static inline uint64_t task_get_instruction_pointer(struct task_state *task_state) {
    return task_state->stack_state.rip;
}

static inline uint64_t task_get_stack_pointer(struct task_state *task_state) {
    return task_state->stack_state.rsp;
}

static inline uint64_t task_get_base_pointer(struct task_state *task_state) {
    return task_state->cpu_state.rbp;
}

static inline enum sc_number task_get_sys_call_number(struct task_state *task_state) {
    return (enum sc_number) task_state->cpu_state.rax;
}

static inline uint64_t task_get_sys_call_arg1(struct task_state *task_state) {
    return task_state->cpu_state.rdi;
}

static inline uint64_t task_get_sys_call_arg2(struct task_state *task_state) {
    return task_state->cpu_state.rsi;
}

static inline uint64_t task_get_sys_call_arg3(struct task_state *task_state) {
    return task_state->cpu_state.rdx;
}

static inline uint64_t task_get_sys_call_arg4(struct task_state *task_state) {
    return task_state->cpu_state.r8;
}

static inline uint64_t task_get_sys_call_arg5(struct task_state *task_state) {
    return task_state->cpu_state.r9;
}

static inline uint64_t task_get_sys_call_arg6(struct task_state *task_state) {
    return task_state->cpu_state.r10;
}

static inline void task_set_sys_call_return_value(struct task_state *task_state, uint64_t value) {
    task_state->cpu_state.rax = value;
}

#endif /* _KERNEL_ARCH_X86_64_ARCH_PROC_TASK_H */
