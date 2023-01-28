#include <kernel/arch/x86/proc/fpu.h>
#include <kernel/proc/task.h>

void task_align_fpu(struct task *task) {
    uintptr_t unaligned_fpu = (uintptr_t) &task->fpu.raw_fpu_state;
    task->fpu.aligned_state = (uint8_t *) ((unaligned_fpu & ~0xFUL) + 16UL);
    assert(((uintptr_t) task->fpu.aligned_state) % 16 == 0);
    assert((uintptr_t) task->fpu.aligned_state >= unaligned_fpu &&
           (uintptr_t) task->fpu.aligned_state <= (uintptr_t) task->fpu.raw_fpu_state.image);
}
