#include <stdlib.h>
#include <stdbool.h>

#include <kernel/mem/page.h>
#include <kernel/proc/process.h>
#include <kernel/arch/x86_64/proc/process.h>
#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/gdt.h>

void arch_init_kernel_process(struct process *kernel_process) {
    kernel_process->arch_process.process_state.stack_state.rip = (uint64_t) &abort;
    kernel_process->arch_process.process_state.stack_state.cs = CS_SELECTOR; 
}

void arch_load_process(struct process *process, uintptr_t entry) {
    process->arch_process.process_state.cpu_state.rbx = 0x332244;
    process->arch_process.cr3 = get_cr3();
    process->arch_process.kernel_stack = KERNEL_PROC_STACK_START;
    process->arch_process.process_state.stack_state.rip = entry;
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    process->arch_process.process_state.stack_state.rflags = get_rflags();
    process->arch_process.process_state.stack_state.rsp = get_vm_region(process->process_memory, VM_PROCESS_STACK)->end;
    process->arch_process.process_state.stack_state.ss = USER_DATA_SELECTOR;

    struct vm_region *kernel_proc_stack = calloc(1, sizeof(struct vm_region));
    kernel_proc_stack->flags = VM_WRITE | VM_NO_EXEC;
    kernel_proc_stack->type = VM_KERNEL_STACK;
    kernel_proc_stack->end = KERNEL_PROC_STACK_START;
    kernel_proc_stack->start = kernel_proc_stack->end - PAGE_SIZE;
    process->process_memory = add_vm_region(process->process_memory, kernel_proc_stack);
    map_vm_region(kernel_proc_stack);
}

void arch_run_process(struct process *process) {
    set_tss_stack_pointer(process->arch_process.kernel_stack);
    load_cr3(process->arch_process.cr3);
    
    __run_process(&process->arch_process);
}