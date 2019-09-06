#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include <kernel/mem/page.h>
#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>
#include <kernel/hal/hal.h>
#include <kernel/arch/x86_64/proc/process.h>
#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/gdt.h>

extern void KERNEL_VM_STACK_START();

#define __KERNEL_VM_STACK_START ((uint64_t) &KERNEL_VM_STACK_START)

static void kernel_idle() {
    while (1);
    __builtin_unreachable();
}

void arch_init_kernel_process(struct process *kernel_process) {
    kernel_process->arch_process.process_state.stack_state.rip = (uint64_t) &kernel_idle;
    kernel_process->arch_process.process_state.stack_state.cs = CS_SELECTOR;
    kernel_process->arch_process.process_state.stack_state.rflags = get_rflags() | (1 << 9);
    kernel_process->arch_process.process_state.stack_state.ss = DATA_SELECTOR;
    kernel_process->arch_process.process_state.stack_state.rsp = __KERNEL_VM_STACK_START;
}

void arch_load_process(struct process *process, uintptr_t entry) {
    process->arch_process.cr3 = get_cr3();
    process->arch_process.kernel_stack = KERNEL_PROC_STACK_START;
    process->arch_process.process_state.cpu_state.rbp = KERNEL_PROC_STACK_START;
    process->arch_process.process_state.stack_state.rip = entry;
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    process->arch_process.process_state.stack_state.rflags = get_rflags() | (1 << 9);
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

void arch_free_process(struct process *process) {
    remove_paging_structure(process->arch_process.cr3, process->process_memory);
}