#include <stdbool.h>

#include <kernel/hal/processor.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/util/validators.h>

struct stack_frame {
    struct stack_frame *next;
    uintptr_t rip;
};

static int do_validate_read(void *addr, size_t size, bool kernel) {
    if (kernel) {
        return validate_kernel_read(addr, size);
    }
    return validate_read(addr, size);
}

static int stack_frame_validate(struct stack_frame *frame, bool kernel) {
    return do_validate_read(frame, sizeof(struct stack_frame), kernel);
}

void proc_record_profile_stack(struct task_state *task_state) {
    struct task *current = get_current_task();
    struct process *process = current->process;

    bool in_kernel = current->in_kernel;

    uintptr_t rip;
    uintptr_t rbp;
    uintptr_t rsp;
    if (task_state) {
        // Called from IRQ context (preemption)
        rip = task_state->stack_state.rip;
        rbp = task_state->cpu_state.rbp;
        rsp = task_state->stack_state.rsp;
    } else if (!in_kernel) {
        // Called from CPU fault context (#GP or #PF)
        rip = current->arch_task.task_state.stack_state.rip;
        rbp = current->arch_task.task_state.cpu_state.rbp;
        rsp = current->arch_task.task_state.stack_state.rsp;
    } else {
        // Called at program exit time
        rip = (uintptr_t) proc_record_profile_stack;
        asm volatile("mov %%rbp, %0" : "=r"(rbp) : : "memory");
        asm volatile("mov %%rsp, %0" : "=r"(rsp) : : "memory");
    }

    char raw_buffer[sizeof(struct profile_event_stack_trace) + PROFILE_MAX_STACK_FRAMES * sizeof(uintptr_t)];
    struct profile_event_stack_trace *ev = (void *) raw_buffer;
    ev->type = PEV_STACK_TRACE;
    ev->count = 0;

    for (;;) {
        if (ev->count < PROFILE_MAX_STACK_FRAMES) {
            ev->frames[ev->count++] = rip;
        }

        if (ev->count < PROFILE_MAX_STACK_FRAMES) {
            // Some leaf functions don't create a stack frame. If so, rsp will point to the return address.
            // Unfortunately, this can produce false positives, so this might not be the best method of
            // stack unwinding.
            if (!do_validate_read((uintptr_t *) rsp, sizeof(uintptr_t), in_kernel)) {
                uintptr_t possible_return_address = *(uintptr_t *) rsp;
                struct vm_region *region = in_kernel ? find_kernel_vm_region_by_addr(possible_return_address)
                                                     : find_user_vm_region_by_addr(possible_return_address);
                if (region && !(region->flags & VM_NO_EXEC)) {
                    ev->frames[ev->count++] = possible_return_address;
                }
            }
        }

        struct stack_frame *frame = (struct stack_frame *) rbp;
        while (!stack_frame_validate(frame, in_kernel) && frame && frame->rip && ev->count < PROFILE_MAX_STACK_FRAMES) {
            ev->frames[ev->count++] = frame->rip;
            frame = frame->next;
        }

        if (!in_kernel || process->in_execve) {
            break;
        }

        // Switch over to the user stack.
        in_kernel = false;
        rip = current->arch_task.user_task_state->stack_state.rip;
        rbp = current->arch_task.user_task_state->cpu_state.rbp;
        rsp = current->arch_task.user_task_state->stack_state.rsp;
    }

    proc_write_profile_buffer(process, raw_buffer, PEV_STACK_TRACE_SIZE(ev));
}
