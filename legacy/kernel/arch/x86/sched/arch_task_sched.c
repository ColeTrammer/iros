#include <string.h>

#include <kernel/arch/x86/asm_utils.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

void sched_tick(struct task_state *task_state) {
    struct task *current_task = get_current_task();
    memcpy(&current_task->arch_task.task_state, task_state, sizeof(struct task_state));
    // debug_log("~tick eip: [ %p ]\n", (void *) task_state->stack_state.esp);
    if (!current_task->kernel_task) {
        fxsave(current_task->fpu.aligned_state);
    }
    if (current_task->sched_ticks_remaining) {
        current_task->sched_ticks_remaining--;
    }
    sched_maybe_yield();
}

/* Must be called from unpremptable context */
void arch_sched_run_next(struct task_state *task_state) {
    struct task *current_task = get_current_task();
    memcpy(&current_task->arch_task.task_state, task_state, sizeof(struct task_state));
    if (!current_task->kernel_task) {
        fxsave(current_task->fpu.aligned_state);
    }
    sched_run_next();
}
