#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/sched/process_sched.h>
#include <kernel/proc/process.h>
#include <kernel/mem/vm_allocator.h>

#include <kernel/arch/arch.h>
#include ARCH_SPECIFIC(asm_utils.h)

void init_irq_handlers() {
    register_irq_handler(&handle_invalid_opcode_entry, 6, false);
    register_irq_handler(&handle_device_not_available_entry, 7, false);
    register_irq_handler(&handle_double_fault_entry, 8, false);
    register_irq_handler(&handle_general_protection_fault_entry, 13, false);
    register_irq_handler(&handle_page_fault_entry, 14, false);
    register_irq_handler(&handle_fpu_exception_entry, 16, false);

    register_irq_handler(&sys_call_entry, 128, true);

    debug_log("Finished Initializing Handlers\n");
}

void handle_double_fault() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Double Fault");
    abort();
}

void handle_general_protection_fault(struct stack_state *stack, uintptr_t error) {
    // FIXME: need to save process state here in case a signal handler is called and returned

    struct process *current = get_current_process();
    debug_log("%d #GP: [ %#.16lX, %lu ]\n", current->pid, stack->rip, error);
    signal_process(current->pid, SIGSEGV);

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error: %lX\033[0m\n", "General Protection Fault", error);
    abort();
}

void handle_page_fault(struct process_state *process_state, uintptr_t address, uintptr_t error) {
    // FIXME: the error code pushed to the stack ruins the process_state struct
    // FIXME: need to save process state here in case a signal handler is called and returned
    struct process *current = get_current_process();
    debug_log("%d page faulted: [ %#.16lX, %#.16lX, %lu ]\n", current->pid, process_state->stack_state.rip, address, error);

    // In this case we just extend the stack
    struct vm_region *vm_stack = get_vm_region(current->process_memory, VM_PROCESS_STACK);
    if (vm_stack && current->pid != 1 && address >= vm_stack->end - 32 * PAGE_SIZE && address <= vm_stack->start) {
        size_t num_pages = NUM_PAGES(address, vm_stack->start);
        assert(num_pages > 0);
        assert(extend_vm_region_start(current->process_memory, VM_PROCESS_STACK, num_pages) == 0);
        assert(vm_stack->start <= address);
        for (size_t i = 0; i < num_pages; i++) {
            map_page(vm_stack->start + i * PAGE_SIZE, VM_NO_EXEC | VM_WRITE | VM_USER);
        }
        return;
    }

    if (vm_stack && current->pid != 1 && !current->in_kernel) {
        signal_process(current->pid, SIGSEGV);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error %lX\n", "Page Fault", error);
    printf("Address: %#.16lX\n", address);
    printf("Process: %d\033[0m\n", get_current_process()->pid);
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

static struct process *last_saved = NULL;

void invalidate_last_saved(struct process *process) {
    if (last_saved == process) {
        last_saved = NULL;
    }
}

void handle_device_not_available() {
    struct process *current = get_current_process();
    assert(current);

    if (last_saved == current) {
        return;
    }

    if (last_saved == NULL) {
        last_saved = current;
        fninit();
        return;
    } else {
        fxsave(last_saved->fpu.raw_fpu_state.image);
    }

    if (!last_saved->fpu.saved) {
        last_saved->fpu.saved = true;
    }

    if (current->fpu.saved) {
        fxrstor(last_saved->fpu.raw_fpu_state.image);
    } else {
        fninit();
    }

    return;
}