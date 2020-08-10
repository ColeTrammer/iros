#include <stdbool.h>

#include <kernel/hal/processor.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/util/validators.h>

struct stack_frame {
    struct stack_frame *next;
    uintptr_t rip;
};

static int stack_frame_validate(struct stack_frame *frame, bool kernel) {
    if (kernel) {
        return validate_kernel_read(frame, sizeof(struct stack_frame));
    }
    return validate_read(frame, sizeof(struct stack_frame));
}

void proc_record_profile_stack(void) {
    struct task *current = get_current_task();
    struct process *process = current->process;

    bool in_kernel = current->in_kernel;
    uintptr_t rip = current->arch_task.task_state.stack_state.rip;
    uintptr_t rbp = current->arch_task.task_state.cpu_state.rbp;

    char raw_buffer[sizeof(struct profile_event_stack_trace) + PROFILE_MAX_STACK_FRAMES * sizeof(uintptr_t)];
    struct profile_event_stack_trace *ev = (void *) raw_buffer;
    ev->type = PEV_STACK_TRACE;
    ev->count = 0;

    for (;;) {
        if (ev->count < PROFILE_MAX_STACK_FRAMES) {
            ev->frames[ev->count++] = rip;
        }
        struct stack_frame *frame = (struct stack_frame *) rbp;
        while (!stack_frame_validate(frame, in_kernel) && frame && frame->rip && ev->count < PROFILE_MAX_STACK_FRAMES) {
            ev->frames[ev->count++] = frame->rip;
            frame = frame->next;
        }

        if (!in_kernel) {
            break;
        }

        // Switch over to the user stack.
        in_kernel = false;
        rip = current->arch_task.user_task_state->stack_state.rip;
        rbp = current->arch_task.user_task_state->cpu_state.rbp;
    }

    proc_write_profile_buffer(process, raw_buffer, PEV_STACK_TRACE_SIZE(ev));
}
