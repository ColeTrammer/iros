#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/pid.h>
#include <kernel/sched/process_sched.h>

#include <kernel/hal/output.h>
#include <kernel/arch/x86_64/proc/process.h>

void arch_sys_print(struct process_state *process_state) {
    debug_log("Sys Print Called: [ %#.16lX, %#.16lX ]\n", process_state->cpu_state.rsi, process_state->cpu_state.rdx);

    bool res = kprint((char*) process_state->cpu_state.rsi, process_state->cpu_state.rdx);
    process_state->cpu_state.rax = res;
}

void arch_sys_exit(struct process_state *process_state) {
    int status = (int) process_state->cpu_state.rsi;
    debug_log("Sys Exit Called: [ %d ]\n", status);

    struct process *process = get_current_process();
    process->sched_state = EXITING;

    sched_run_next();
}

void arch_sys_sbrk(struct process_state *process_state) {
    intptr_t increment = process_state->cpu_state.rsi;
    void *res;
    if (increment < 0) {
        res = add_vm_pages_end(0, VM_PROCESS_HEAP);
        remove_vm_pages_end(-increment, VM_PROCESS_HEAP);
    } else {
        res = add_vm_pages_end(increment, VM_PROCESS_HEAP);
    }
    process_state->cpu_state.rax = (uint64_t) res;
}