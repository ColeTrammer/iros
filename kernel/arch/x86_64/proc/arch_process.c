#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>

#include <kernel/mem/page.h>
#include <kernel/mem/kernel_vm.h>
#include <kernel/proc/process.h>
#include <kernel/sched/process_sched.h>
#include <kernel/hal/hal.h>
#include <kernel/arch/x86_64/proc/process.h>
#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/gdt.h>
#include <kernel/hal/output.h>

/* Default Args And Envp Passed to First Program */
static char *test_argv[2] = {
    "shell", NULL
};

static char *test_envp[2] = {
    "OS=os_2", NULL
};

static void kernel_idle() {
    disable_interrupts();
    sched_run_next();
}

void arch_init_kernel_process(struct process *kernel_process) {
    /* Sets Up Kernel Process To Idle */
    kernel_process->arch_process.process_state.stack_state.rip = (uint64_t) &kernel_idle;
    kernel_process->arch_process.process_state.stack_state.cs = CS_SELECTOR;
    kernel_process->arch_process.process_state.stack_state.rflags = get_rflags() | INTERRUPS_ENABLED_FLAG;
    kernel_process->arch_process.process_state.stack_state.ss = DATA_SELECTOR;
    kernel_process->arch_process.process_state.stack_state.rsp = __KERNEL_VM_STACK_START;
    kernel_process->arch_process.setup_kernel_stack = false;
    kernel_process->fpu.saved = false;
}

void arch_load_process(struct process *process, uintptr_t entry) {
    process->arch_process.cr3 = get_cr3();
    process->arch_process.kernel_stack = KERNEL_PROC_STACK_START;
    process->arch_process.process_state.cpu_state.rbp = KERNEL_PROC_STACK_START;
    process->arch_process.process_state.stack_state.rip = entry;
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    process->arch_process.process_state.stack_state.rflags = get_rflags() | INTERRUPS_ENABLED_FLAG;
    process->arch_process.process_state.stack_state.rsp = map_program_args(get_vm_region(process->process_memory, VM_PROCESS_STACK)->end, test_argv, test_envp);
    process->arch_process.process_state.stack_state.ss = USER_DATA_SELECTOR;

    struct vm_region *kernel_proc_stack = calloc(1, sizeof(struct vm_region));
    kernel_proc_stack->flags = VM_WRITE | VM_NO_EXEC;
    kernel_proc_stack->type = VM_KERNEL_STACK;
    kernel_proc_stack->end = KERNEL_PROC_STACK_START;
    kernel_proc_stack->start = kernel_proc_stack->end - PAGE_SIZE;
    process->process_memory = add_vm_region(process->process_memory, kernel_proc_stack);

    /* Map Process Stack To Reserve Pages For It, But Then Unmap It So That Other Processes Can Do The Same (Each Process Loads Its Own Stack Before Execution) */
    process->arch_process.kernel_stack_info = map_page_with_info(kernel_proc_stack->start, kernel_proc_stack->flags);
    do_unmap_page(kernel_proc_stack->start, false);
    process->arch_process.setup_kernel_stack = false;
    process->fpu.saved = false;
}

/* Must be called from unpremptable context */
void arch_run_process(struct process *process) {
    set_tss_stack_pointer(process->arch_process.kernel_stack);
    load_cr3(process->arch_process.cr3);

    /* Stack Set Up Occurs Here Because Sys Calls Use That Memory In Their Own Stack And We Can Only Write Pages When They Are Mapped In Currently */
    if (process->arch_process.setup_kernel_stack) {
        struct vm_region *kernel_stack = get_vm_region(process->process_memory, VM_KERNEL_STACK);
        do_unmap_page(kernel_stack->start, false);
        process->arch_process.kernel_stack_info = map_page_with_info(kernel_stack->start, kernel_stack->flags);
        process->arch_process.setup_kernel_stack = false;
    } else if (process->arch_process.kernel_stack_info != NULL) {
        map_page_info(process->arch_process.kernel_stack_info);
    }

    __run_process(&process->arch_process);
}

/* Must be called from unpremptable context */
void arch_free_process(struct process *process, bool free_paging_structure) {
    if (free_paging_structure) {
        map_page_info(process->arch_process.kernel_stack_info);
        remove_paging_structure(process->arch_process.cr3, process->process_memory);
    }

    free(process->arch_process.kernel_stack_info);
}

bool proc_in_kernel(struct process *process) {
    return process->in_kernel;
}

void proc_do_sig_handler(struct process *process, int signum) {
    assert(process->sig_state[signum].sa_handler != SIG_IGN);
    assert(process->sig_state[signum].sa_handler != SIG_DFL);

    set_tss_stack_pointer(process->arch_process.kernel_stack);
    load_cr3(process->arch_process.cr3);

    /* Stack Set Up Occurs Here Because Sys Calls Use That Memory In Their Own Stack And We Can Only Write Pages When They Are Mapped In Currently */
    if (process->arch_process.setup_kernel_stack) {
        struct vm_region *kernel_stack = get_vm_region(process->process_memory, VM_KERNEL_STACK);
        do_unmap_page(kernel_stack->start, false);
        process->arch_process.kernel_stack_info = map_page_with_info(kernel_stack->start, kernel_stack->flags);
        process->arch_process.setup_kernel_stack = false;
    } else if (process->arch_process.kernel_stack_info != NULL) {
        map_page_info(process->arch_process.kernel_stack_info);
    }

    // FIXME: Currently doesn't save fpu information
    uint64_t save_rsp = proc_in_kernel(process) ? 
                        process->arch_process.process_state.cpu_state.user_rsp : 
                        process->arch_process.process_state.stack_state.rsp;
    debug_log("Save RSP: [ %#.16lX ]\n", save_rsp);
    debug_log("In Kernel: [ %d ]\n", proc_in_kernel(process));
    debug_log("RSP: [ %#.16lX, %#.16lX ]\n", process->arch_process.process_state.cpu_state.user_rsp, process->arch_process.process_state.stack_state.rsp);

    assert(save_rsp != 0);
    struct process_state *save_state = ((struct process_state*) ((save_rsp - 128) & ~0xF)) - 1; // Sub 128 to enforce red-zone
    memcpy(save_state, &process->arch_process.process_state, sizeof(struct process_state));

    struct sigaction act = process->sig_state[signum];
    process->arch_process.process_state.stack_state.rip = (uintptr_t) act.sa_handler;
    process->arch_process.process_state.cpu_state.rdi = signum;
    process->arch_process.process_state.stack_state.rsp = (uintptr_t) save_state;
    process->arch_process.process_state.stack_state.ss = USER_DATA_SELECTOR;
    process->arch_process.process_state.stack_state.cs = USER_CODE_SELECTOR;
    run_process(process);
}
