#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/pid.h>
#include <kernel/sched/process_sched.h>

#include <kernel/hal/hal.h>
#include <kernel/hal/output.h>
#include <kernel/arch/x86_64/proc/process.h>

void arch_sys_print(struct process_state *process_state) {
    // disable_interrupts();

    screen_print((char*) process_state->cpu_state.rsi, process_state->cpu_state.rdx);

    process_state->cpu_state.rax = true;
}

void arch_sys_exit(struct process_state *process_state) {
    disable_interrupts();

    struct process *process = get_current_process();
    process->sched_state = EXITING;

    int exit_code = (int) process_state->cpu_state.rsi;
    debug_log("Process Exited: [ %d, %d ]\n", process->pid, exit_code);

    enable_interrupts();

    while (1);
}

void arch_sys_sbrk(struct process_state *process_state) {
    disable_interrupts();

    intptr_t increment = process_state->cpu_state.rsi;
    void *res;
    if (increment < 0) {
        res = add_vm_pages_end(0, VM_PROCESS_HEAP);
        remove_vm_pages_end(-increment, VM_PROCESS_HEAP);
    } else {
        res = add_vm_pages_end(increment, VM_PROCESS_HEAP);
    }
    process_state->cpu_state.rax = (uint64_t) res;

    enable_interrupts();
}