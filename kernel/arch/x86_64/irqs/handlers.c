#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/sched/task_sched.h>
#include <kernel/proc/task.h>
#include <kernel/mem/vm_allocator.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(asm_utils.h)

void init_irq_handlers() {
    register_irq_handler(&handle_invalid_opcode_entry, 6, false, false);
    register_irq_handler(&handle_device_not_available_entry, 7, false, false);
    register_irq_handler(&handle_double_fault_entry, 8, false, true);
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

void handle_general_protection_fault(struct task_interrupt_state *task_state) {
    struct task *current = get_current_task();
    debug_log("%d #GP: [ %#.16lX, %lu ]\n", current->process->pid, task_state->stack_state.rip, task_state->error_code);

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGSEGV); // You can't block this so we don't check
    
        // If we get here, the task that faulted was just send a terminating signal
        sched_run_next();
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error: %lX\033[0m\n", "General Protection Fault", task_state->error_code);
    abort();
}

void handle_page_fault(struct task_interrupt_state *task_state, uintptr_t address) {
    struct task *current = get_current_task();
    debug_log("%d page faulted: [ %#.16lX, %#.16lX, %#.16lX, %lu ]\n", current->process->pid, 
        task_state->stack_state.rsp, task_state->stack_state.rip, address, task_state->error_code);

    // In this case we just extend the stack
    struct vm_region *vm_stack = get_vm_region(current->process->process_memory, VM_TASK_STACK);
    if (vm_stack && !current->kernel_task && address >= vm_stack->end - 32 * PAGE_SIZE && address <= vm_stack->start) {
        size_t num_pages = NUM_PAGES(address, vm_stack->start);
        assert(num_pages > 0);
        assert(extend_vm_region_start(current->process->process_memory, VM_TASK_STACK, num_pages) == 0);
        assert(vm_stack->start <= address);
        for (size_t i = 0; i < num_pages; i++) {
            map_page(vm_stack->start + i * PAGE_SIZE, VM_NO_EXEC | VM_WRITE | VM_USER);
        }
        return;
    }

    if (vm_stack && !current->kernel_task && !current->in_kernel) {
        memcpy(&current->arch_task.task_state.cpu_state, &task_state->cpu_state, sizeof(struct cpu_state));
        memcpy(&current->arch_task.task_state.stack_state, &task_state->stack_state, sizeof(struct stack_state));
        task_do_sig(current, SIGSEGV); // You can't block this so we don't check
        
        // If we get here, the task that faulted was just send a terminating signal
        sched_run_next();
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error %lX\n", "Page Fault", task_state->error_code);
    printf("Address: %#.16lX\n", address);
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

static struct task *last_saved = NULL;

void invalidate_last_saved(struct task *task) {
    if (last_saved == task) {
        last_saved = NULL;
    }
}

void handle_device_not_available() {
    struct task *current = get_current_task();
    assert(current);

    if (last_saved == current) {
        return;
    }

    if (last_saved == NULL) {
        last_saved = current;
        fninit();
        return;
    } else {
        fxsave(last_saved->fpu.aligned_state);
    }

    if (!last_saved->fpu.saved) {
        last_saved->fpu.saved = true;
    }

    if (current->fpu.saved) {
        fxrstor(last_saved->fpu.aligned_state);
    } else {
        fninit();
    }

    return;
}