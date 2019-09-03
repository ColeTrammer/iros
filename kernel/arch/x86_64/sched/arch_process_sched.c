#include <string.h>

#include <kernel/sched/process_sched.h>
#include <kernel/hal/timer.h>
#include <kernel/arch/x86_64/asm_utils.h>

void arch_init_process_sched() {
    register_callback(&arch_sched_run_next, 1);
}

void arch_sched_run_next(struct process_state *process_state) {
    struct process *current_process = get_current_process();
    memcpy(&current_process->arch_process.process_state, process_state, sizeof(struct process_state));

    sched_run_next();
}