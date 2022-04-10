#ifndef _KERNEL_ARCH_I686_PROC_ARCH_TASK_H
#define _KERNEL_ARCH_I686_PROC_ARCH_TASK_H 1

#include <stdbool.h>
#include <stdint.h>

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
    struct task_state *user_task_state;
    void *user_thread_pointer;
};

#define FPU_IMAGE_SIZE 512

// Can be longer if more extensions are enabled,
// so this basically needs to be variable length
struct raw_fpu_state {
    uint8_t padding[16];
    uint8_t image[FPU_IMAGE_SIZE];
} __attribute__((packed));

struct arch_fpu_state {
    struct raw_fpu_state raw_fpu_state;
    uint8_t *aligned_state;
};

struct task;

void task_align_fpu(struct task *task);
void task_setup_user_state(struct task_state *task_state);

extern void __run_task(struct arch_task *state);

static inline void task_set_ip(struct task_state *task_state, uintptr_t ip) {
    task_state->stack_state.rip = ip;
}

static inline void task_set_sp(struct task_state *task_state, uintptr_t sp) {
    task_state->stack_state.rsp = sp;
}

#endif /* _KERNEL_ARCH_I686_ARCH_PROC_TASK_H */
