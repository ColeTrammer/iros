#include <kernel/proc/process.h>

#include "../../../hal/x86_64/gdt.h"

void arch_load_process(struct process *process, uintptr_t entry) {
    process->arch_process.process_state.stack_state.rip = entry;
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    process->arch_process.process_state.stack_state.rflags = get_rflags();
    process->arch_process.process_state.stack_state.rsp = get_vm_region(process->process_memory, VM_PROCESS_STACK)->end;
    process->arch_process.process_state.stack_state.ss = USER_DATA_SELECTOR;
}

void arch_run_process(struct process *process) {
    __run_process(&process->arch_process);
}
