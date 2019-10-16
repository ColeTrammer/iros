#include <string.h>

#include <kernel/sched/process_sched.h>
#include <kernel/hal/timer.h>
#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/output.h>

void arch_init_process_sched() {
    set_sched_callback(&arch_sched_run_next, 5);
}

/* Must be called from unpremptable context */
void arch_sched_run_next(struct process_state *process_state) {
    struct process *current_process = get_current_process();
    debug_log("Saving state: [ %d, %#.16lX ]\n", proc_in_kernel(current_process), process_state->stack_state.rsp);
    uint64_t user_save = proc_in_kernel(current_process) ?
                         current_process->arch_process.process_state.cpu_state.user_rsp :
                         process_state->stack_state.rsp;
    memcpy(&current_process->arch_process.process_state, process_state, sizeof(struct process_state));
    current_process->arch_process.process_state.cpu_state.user_rsp = user_save;

    sched_run_next();
}