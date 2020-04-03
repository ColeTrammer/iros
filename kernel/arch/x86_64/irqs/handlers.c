#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_object.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

// #define PAGE_FAULT_DEBUG
// #define PAGING_DEBUG
#define DEVICE_NOT_AVAILABLE_DEBUG

void init_irq_handlers() {
    register_irq_handler(&handle_divide_by_zero, 0, false, false);
    register_irq_handler(&handle_debug_entry, 1, false, false);
    register_irq_handler(&handle_non_maskable_interrupt_entry, 2, false, false);
    register_irq_handler(&handle_breakpoint_entry, 3, false, false);
    register_irq_handler(&handle_overflow_entry, 4, false, false);
    register_irq_handler(&handle_bound_range_exceeded_entry, 5, false, false);
    register_irq_handler(&handle_invalid_opcode_entry, 6, false, false);
    register_irq_handler(&handle_device_not_available_entry, 7, false, false);
    register_irq_handler(&handle_double_fault_entry, 8, false, true);

    register_irq_handler(&handle_invalid_tss_entry, 10, false, false);
    register_irq_handler(&handle_segment_not_present_entry, 11, false, false);
    register_irq_handler(&handle_stack_fault, 12, false, true);
    register_irq_handler(&handle_general_protection_fault_entry, 13, false, false);
    register_irq_handler(&handle_page_fault_entry, 14, false, false);

    register_irq_handler(&handle_fpu_exception_entry, 16, false, false);
    register_irq_handler(&handle_alignment_check_entry, 17, false, false);
    register_irq_handler(&handle_machine_check_entry, 18, false, false);
    register_irq_handler(&handle_simd_exception_entry, 19, false, false);
    register_irq_handler(&handle_virtualization_exception, 20, false, false);

    register_irq_handler(&handle_security_exception, 30, false, false);

    register_irq_handler(&sys_call_entry, 128, true, false);

    debug_log("Finished Initializing Handlers\n");
}

void handle_double_fault() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Double Fault");
    abort();
}

void handle_divide_by_zero(struct task_state *task_state) {
    struct task *current = get_current_task();
    debug_log("%d #DE: [ %#.16lX ]\n", current->process->pid, task_state->stack_state.rip);

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGFPE); // You can't block this so we don't check

        // If we get here, the task that faulted was just sent a terminating signal
        sys_sched_run_next(&current->arch_task.task_state);
    }

    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Divide by Zero Error");
    abort();
}

void handle_stack_fault(struct task_interrupt_state *task_state) {
    struct task *current = get_current_task();
    debug_log("%d #SF: [ %#.16lX, %lu ]\n", current->process->pid, task_state->stack_state.rip, task_state->error_code);

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGSEGV); // You can't block this so we don't check

        // If we get here, the task that faulted was just sent a terminating signal
        sys_sched_run_next(&current->arch_task.task_state);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error: %lX\033[0m\n", "Stack Fault", task_state->error_code);
    abort();
}

void handle_general_protection_fault(struct task_interrupt_state *task_state) {
    struct task *current = get_current_task();
    debug_log("%d #GP: [ %#.16lX, %lu ]\n", current->process->pid, task_state->stack_state.rip, task_state->error_code);

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGSEGV); // You can't block this so we don't check

        // If we get here, the task that faulted was just sent a terminating signal
        sys_sched_run_next(&current->arch_task.task_state);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error: %lX\n", "General Protection Fault", task_state->error_code);
    printf("RIP: %#.16lX\n", task_state->stack_state.rip);
    printf("task: %d\033[0m\n", get_current_task()->process->pid);
    abort();
}

void handle_page_fault(struct task_interrupt_state *task_state, uintptr_t address) {
    struct task *current = get_current_task();
    // In this case we just need to map in a region that's allocation was put off by the kernel
    struct vm_region *vm_region = find_user_vm_region_by_addr(address);

#ifdef PAGE_FAULT_DEBUG
    debug_log("%d page faulted: [ %#.16lX, %#.16lX, %#.16lX, %lu, %p ]\n", current->process->pid, task_state->stack_state.rsp,
              task_state->stack_state.rip, address, task_state->error_code, vm_region);
#endif /* PAGE_FAULT_DEBUG */

    if (vm_region && !(task_state->error_code & 1) && address != vm_region->end && !(vm_region->flags & VM_PROT_NONE) &&
        !(vm_region->flags & VM_COW)) {
        // Return if we can handle the fault
        if (!vm_handle_fault_in_region(vm_region, address)) {
            return;
        }
    }

#ifdef PAGING_DEBUG
    uint64_t pml4_offset = (address >> 39) & 0x1FF;
    uint64_t pdp_offset = (address >> 30) & 0x1FF;
    uint64_t pd_offset = (address >> 21) & 0x1FF;
    uint64_t pt_offset = (address >> 12) & 0x1FF;

    uint64_t *pml4_entry = PML4_BASE + pml4_offset;
    uint64_t *pdp_entry = PDP_BASE + (0x1000 * pml4_offset) / sizeof(uint64_t) + pdp_offset;
    uint64_t *pd_entry = PD_BASE + (0x200000 * pml4_offset + 0x1000 * pdp_offset) / sizeof(uint64_t) + pd_offset;
    uint64_t *pt_entry = PT_BASE + (0x40000000 * pml4_offset + 0x200000 * pdp_offset + 0x1000 * pd_offset) / sizeof(uint64_t) + pt_offset;

    debug_log("*pml4_entry: [ %#.16lX ]\n", *pml4_entry);
    if (*pml4_entry & 1) {
        debug_log("*pdp_entry: [ %#.16lX ]\n", *pdp_entry);
        if (*pdp_entry & 1) {
            debug_log("*pd_entry: [ %#.16lX ]\n", *pd_entry);
            if (*pd_entry & 1) {
                debug_log("*pt_entry: [ %#.16lX ]\n", *pt_entry);
            }
        }
    }
#endif /* PAGING_DEBUG */

    bool is_kernel = current->kernel_task || current->in_kernel;
    printf("\033[32mProcess \033[37m(\033[34m %d:%d \033[37m): \033[1;31mCRASH (page fault)\033[0;37m: [ %#.16lX, %#.16lX, %#.16lX ]\n",
           current->process->pid, current->tid, address, task_state->stack_state.rip, task_state->stack_state.rsp);
    if (!is_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGSEGV); // You can't block this so we don't check

        // If we get here, the task that faulted was just sent a terminating signal
        sys_sched_run_next(&current->arch_task.task_state);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_process_regions(current->process);
    dump_kernel_regions(address);

    kernel_stack_trace(task_state->stack_state.rip, task_state->cpu_state.rbp);
    abort();
}

void handle_invalid_opcode() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Invalid Opcode");
    abort();
}

void handle_fpu_exception() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "FPU Exception");
    abort();
}

void handle_device_not_available() {
#ifdef DEVICE_NOT_AVAILABLE_DEBUG
    struct task *current = get_current_task();
    assert(current);
    debug_log("handling: [ %d:%d ]\n", current->process->pid, current->tid);
#endif /* DEVICE_NOT_AVAILABLE_DEBUG */
}

void handle_debug() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Debug");
    abort();
}

void handle_non_maskable_interrupt() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Non-maskable Interrupt");
    abort();
}

void handle_breakpoint() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Breakpoint");
    abort();
}

void handle_overflow() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Overflow");
    abort();
}

void handle_bound_range_exceeded() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Bound Range Exceeded");
    abort();
}

void handle_invalid_tss() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Invalid TSS");
    abort();
}

void handle_segment_not_present() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Segment Not Present");
    abort();
}

void handle_alignment_check() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Alignment Check");
    abort();
}

void handle_machine_check() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Machine Check");
    abort();
}

void handle_simd_exception() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "SIMD Exception");
    abort();
}

void handle_virtualization_exception() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Virtualization Exception");
    abort();
}

void handle_security_exception() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Security Exception");
    abort();
}