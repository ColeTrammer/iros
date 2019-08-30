#include <stdlib.h>

#include <kernel/proc/process.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/gdt.h>

void arch_init_kernel_process(struct process *kernel_process) {
    kernel_process->arch_process.process_state.stack_state.rip = (uint64_t) &abort;
    kernel_process->arch_process.process_state.stack_state.cs = CS_SELECTOR; 
}

void arch_load_process(struct process *process, uintptr_t entry) {
    process->arch_process.cr3 = get_cr3();
    process->arch_process.process_state.stack_state.rip = entry;
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    process->arch_process.process_state.stack_state.rflags = get_rflags();
    process->arch_process.process_state.stack_state.rsp = get_vm_region(process->process_memory, VM_PROCESS_STACK)->end;
    process->arch_process.process_state.stack_state.ss = USER_DATA_SELECTOR;
}

void arch_run_process(struct process *process) {
    __run_process(&process->arch_process);
}
