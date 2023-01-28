#ifndef _KERNEL_ARCH_X86_PROC_FPU_H
#define _KERNEL_ARCH_X86_PROC_FPU_H 1

#include <stdint.h>

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

#endif /* _KERNEL_ARCH_X86_PROC_FPU_H */
