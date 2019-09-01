#include <stdbool.h>

#include <kernel/hal/output.h>
#include <kernel/arch/x86_64/proc/process.h>

void arch_sys_print(struct process_state *process_state) {
    bool res = kprint((char*) process_state->cpu_state.rsi, process_state->cpu_state.rdx);
    process_state->cpu_state.rax = res;
}