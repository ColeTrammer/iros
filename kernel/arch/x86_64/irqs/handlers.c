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
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

#define PAGE_FAULT_DEBUG
// #define DEVICE_NOT_AVAILABLE_DEBUG

void init_irq_handlers() {
    register_irq_handler(&handle_divide_by_zero, 0, false, false);
    register_irq_handler(&handle_invalid_opcode_entry, 6, false, false);
    register_irq_handler(&handle_device_not_available_entry, 7, false, false);
    register_irq_handler(&handle_double_fault_entry, 8, false, true);
    register_irq_handler(&handle_stack_fault, 12, false, true);
    register_irq_handler(&handle_general_protection_fault_entry, 13, false, false);
    register_irq_handler(&handle_page_fault_entry, 14, false, false);
    register_irq_handler(&handle_fpu_exception_entry, 16, false, false);

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
        sched_run_next();
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
        sched_run_next();
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
        sched_run_next();
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

    if (vm_region && !current->kernel_task && !(task_state->error_code & 1) && address != vm_region->end &&
        !(vm_region->flags & VM_PROT_NONE) && !(vm_region->flags & VM_COW)) {
        // Return if we can handle the fault
        if (!vm_handle_fault_in_region(vm_region, address)) {
            return;
        }
    }

    if (!current->kernel_task && !current->in_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGSEGV); // You can't block this so we don't check

        // If we get here, the task that faulted was just sent a terminating signal
        sched_run_next();
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_kernel_regions(address);

    dump_registers_to_screen();
    printf("\n\033[31m%s: Error %lX\n", "Page Fault", task_state->error_code);
    printf("Address: %#.16lX\n", address);
    printf("RIP: %#.16lX\n", task_state->stack_state.rip);
    printf("task: %d\033[0m\n", get_current_task()->process->pid);
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