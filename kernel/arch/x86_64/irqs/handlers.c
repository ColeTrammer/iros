#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/mem/vm_object.h>
#include <kernel/proc/elf64.h>
#include <kernel/proc/task.h>
#include <kernel/sched/task_sched.h>

// #define PAGE_FAULT_DEBUG
// #define PAGING_DEBUG
// #define DEVICE_NOT_AVAILABLE_DEBUG

static bool handle_double_fault(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Double Fault");
    abort();
    return true;
}

static bool handle_divide_by_zero(struct irq_context *context) {
    struct task_state *task_state = context->task_state;

    struct task *current = get_current_task();
    debug_log("#DE: [ %#.16lX ]\n", task_state->stack_state.rip);

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state, task_state, sizeof(struct task_state));
        spin_lock(&current->sig_lock);
        task_do_sig(current, SIGFPE);
        spin_unlock(&current->sig_lock);

        // If we get here, the task that faulted was just sent a terminating signal
        arch_sched_run_next(&current->arch_task.task_state);
    }

    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Divide by Zero Error");
    abort();
    return true;
}

static bool handle_stack_fault(struct irq_context *context) {
    struct task_state *task_state = context->task_state;
    uint32_t error_code = context->error_code;

    struct task *current = get_current_task();
    debug_log("%d #SF: [ %#.16lX, %u ]\n", current->process->pid, task_state->stack_state.rip, error_code);

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state, task_state, sizeof(struct task_state));
        spin_lock(&current->sig_lock);
        task_do_sig(current, SIGSEGV);
        spin_unlock(&current->sig_lock);

        // If we get here, the task that faulted was just sent a terminating signal
        arch_sched_run_next(&current->arch_task.task_state);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_registers_to_screen();
    debug_log("\n\033[31m%s: Error: %X\033[0m\n", "Stack Fault", error_code);
    abort();
    return true;
}

static bool handle_general_protection_fault(struct irq_context *context) {
    struct task_state *task_state = context->task_state;
    uint32_t error_code = context->error_code;

    struct task *current = get_current_task();
    debug_log("%d #GP: [ %#.16lX, %u ]\n", current->process->pid, task_state->stack_state.rip, error_code);

    debug_log("\n\033[32mProcess \033[37m(\033[34m %d:%d \033[37m): \033[1;31mCRASH (general protection fault)\033[0;37m: [ %#.16lX, "
              "%#.16lX ]\n",
              current->process->pid, current->tid, task_state->stack_state.rip, task_state->stack_state.rsp);
    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state, task_state, sizeof(struct task_state));
        spin_lock(&current->sig_lock);
        task_do_sig(current, SIGSEGV);
        spin_unlock(&current->sig_lock);

        // If we get here, the task that faulted was just sent a terminating signal
        arch_sched_run_next(&current->arch_task.task_state);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_process_regions(current->process);
    dump_kernel_regions(0);

    kernel_stack_trace(task_state->stack_state.rip, task_state->cpu_state.rbp);
    abort();
    return true;
}

static bool handle_page_fault(struct irq_context *context) {
    struct task_state *task_state = context->task_state;
    uint32_t error_code = context->error_code;
    uintptr_t address = get_cr2();

    struct task *current = get_current_task();
    // In this case we just need to map in a region that's allocation was put off by the kernel
    struct vm_region *vm_region = find_user_vm_region_by_addr(address);

#ifdef PAGE_FAULT_DEBUG
    debug_log("%d page faulted: [ %#.16lX, %#.16lX, %#.16lX, %u, %p ]\n", current->process->pid, task_state->stack_state.rsp,
              task_state->stack_state.rip, address, error_code, vm_region);
#endif /* PAGE_FAULT_DEBUG */

    if (vm_region && !(error_code & 1) && address != vm_region->end && !(vm_region->flags & VM_PROT_NONE) && !(vm_region->flags & VM_COW)) {
        // Return if we can handle the fault
        if (!vm_handle_fault_in_region(vm_region, address)) {
            return true;
        }
    }

    if (vm_region && ((error_code & 3) == 3) && address != vm_region->end && is_virt_addr_cow(address) &&
        !(vm_region->flags & VM_PROT_NONE) && (vm_region->flags & VM_WRITE)) {
#ifdef PAGE_FAULT_DEBUG
        debug_log("handling cow fault: [ %#.16lX ]\n", address);
#endif /* PAGE_FAULT_DEBUG */
        if (!vm_handle_cow_fault_in_region(vm_region, address)) {
            return true;
        }
    }

    bool is_kernel = current->kernel_task || current->in_kernel;
    debug_log(
        "\n\033[32mProcess \033[37m(\033[34m %d:%d \033[37m): \033[1;31mCRASH (page fault)\033[0;37m: [ %#.16lX, %#.16lX, %#.16lX, %u ]\n",
        current->process->pid, current->tid, address, task_state->stack_state.rip, task_state->stack_state.rsp, error_code);
    if (!is_kernel) {
        memcpy(&current->arch_task.task_state, task_state, sizeof(struct task_state));
        spin_lock(&current->sig_lock);
        task_do_sig(current, SIGSEGV);
        spin_unlock(&current->sig_lock);

        // If we get here, the task that faulted was just sent a terminating signal
        arch_sched_run_next(&current->arch_task.task_state);
    }

    // We shouldn't get here unless SIGSEGV is blocked???
    dump_process_regions(current->process);
    dump_kernel_regions(address);

    kernel_stack_trace(task_state->stack_state.rip, task_state->cpu_state.rbp);
    abort();
    return true;
}

static bool handle_invalid_opcode(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Invalid Opcode");
    abort();
    return true;
}

static bool handle_fpu_exception(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "FPU Exception");
    abort();
    return true;
}

static bool handle_device_not_available(struct irq_context *context __attribute__((unused))) {
#ifdef DEVICE_NOT_AVAILABLE_DEBUG
    struct task *current = get_current_task();
    assert(current);
    debug_log("handling: [ %d:%d ]\n", current->process->pid, current->tid);
#endif /* DEVICE_NOT_AVAILABLE_DEBUG */
    return true;
}

static bool handle_debug(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Debug");
    abort();
    return true;
}

static bool handle_non_maskable_interrupt(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Non-maskable Interrupt");
    abort();
    return true;
}

static bool handle_breakpoint(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Breakpoint");
    abort();
    return true;
}

static bool handle_overflow(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Overflow");
    abort();
    return true;
}

static bool handle_bound_range_exceeded(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Bound Range Exceeded");
    abort();
    return true;
}

static bool handle_invalid_tss(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Invalid TSS");
    abort();
    return true;
}

static bool handle_segment_not_present(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Segment Not Present");
    abort();
    return true;
}

static bool handle_alignment_check(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Alignment Check");
    abort();
    return true;
}

static bool handle_machine_check(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Machine Check");
    abort();
    return true;
}

static bool handle_simd_exception(struct irq_context *context __attribute__((unused))) {
    struct task_state *task_state = context->task_state;

    struct task *current = get_current_task();
    debug_log("#XF: [ %#.16lX, %#.8X ]\n", task_state->stack_state.rip, get_mxcsr());

    if (!current->in_kernel) {
        memcpy(&current->arch_task.task_state, task_state, sizeof(struct task_state));
        spin_lock(&current->sig_lock);
        task_do_sig(current, SIGFPE);
        spin_unlock(&current->sig_lock);

        // If we get here, the task that faulted was just sent a terminating signal
        arch_sched_run_next(&current->arch_task.task_state);
    }

    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "SIMD Exception");
    abort();
    return true;
}

static bool handle_virtualization_exception(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Virtualization Exception");
    abort();
    return true;
}

static bool handle_security_exception(struct irq_context *context __attribute__((unused))) {
    dump_registers_to_screen();
    debug_log("\n\033[31m%s\033[0m\n", "Security Exception");
    abort();
    return true;
}

static struct irq_handler handle_divide_by_zero_irq = { .handler = &handle_divide_by_zero };
static struct irq_handler handle_debug_irq = { .handler = &handle_debug };
static struct irq_handler handle_non_maskable_interrupt_irq = { .handler = &handle_non_maskable_interrupt };
static struct irq_handler handle_breakpoint_irq = { .handler = &handle_breakpoint };
static struct irq_handler handle_overflow_irq = { .handler = &handle_overflow };
static struct irq_handler handle_bound_range_exceeded_irq = { .handler = &handle_bound_range_exceeded };
static struct irq_handler handle_invalid_opcode_irq = { .handler = &handle_invalid_opcode };
static struct irq_handler handle_device_not_available_irq = { .handler = &handle_device_not_available };
static struct irq_handler handle_double_fault_irq = { .handler = &handle_double_fault };
static struct irq_handler handle_invalid_tss_irq = { .handler = &handle_invalid_tss };
static struct irq_handler handle_segment_not_present_irq = { .handler = &handle_segment_not_present };
static struct irq_handler handle_stack_fault_irq = { .handler = &handle_stack_fault };
static struct irq_handler handle_general_protection_fault_irq = { .handler = &handle_general_protection_fault };
static struct irq_handler handle_page_fault_irq = { .handler = &handle_page_fault };
static struct irq_handler handle_fpu_exception_irq = { .handler = &handle_fpu_exception };
static struct irq_handler handle_alignment_check_irq = { .handler = &handle_alignment_check };
static struct irq_handler handle_machine_check_irq = { .handler = &handle_machine_check };
static struct irq_handler handle_simd_exception_irq = { .handler = &handle_simd_exception };
static struct irq_handler handle_virtualization_exception_irq = { .handler = &handle_virtualization_exception };
static struct irq_handler handle_security_exception_irq = { .handler = &handle_security_exception };
static struct irq_handler sys_call_irq = { .handler = &arch_system_call_entry };

extern struct list_node irq_handlers[255];

void init_irq_handlers() {
    for (size_t i = 0; i < 255; i++) {
        init_list(&irq_handlers[i]);
    }

    register_irq_handler(&handle_divide_by_zero_irq, 0);
    register_irq_handler(&handle_debug_irq, 1);
    register_irq_handler(&handle_non_maskable_interrupt_irq, 2);
    register_irq_handler(&handle_breakpoint_irq, 3);
    register_irq_handler(&handle_overflow_irq, 4);
    register_irq_handler(&handle_bound_range_exceeded_irq, 5);
    register_irq_handler(&handle_invalid_opcode_irq, 6);
    register_irq_handler(&handle_device_not_available_irq, 7);
    register_irq_handler(&handle_double_fault_irq, 8);

    register_irq_handler(&handle_invalid_tss_irq, 10);
    register_irq_handler(&handle_segment_not_present_irq, 11);
    register_irq_handler(&handle_stack_fault_irq, 12);
    register_irq_handler(&handle_general_protection_fault_irq, 13);
    register_irq_handler(&handle_page_fault_irq, 14);

    register_irq_handler(&handle_fpu_exception_irq, 16);
    register_irq_handler(&handle_alignment_check_irq, 17);
    register_irq_handler(&handle_machine_check_irq, 18);
    register_irq_handler(&handle_simd_exception_irq, 19);
    register_irq_handler(&handle_virtualization_exception_irq, 20);

    register_irq_handler(&handle_security_exception_irq, 30);

    register_irq_handler(&sys_call_irq, 128);
}
