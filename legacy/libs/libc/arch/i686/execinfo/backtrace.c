#define __libc_internal

#include <execinfo.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

struct stack_frame {
    struct stack_frame *next;
    void *rip;
};

int backtrace(void **buffer, int buffer_max) {
    uint32_t starting_esp;
    uint32_t starting_ebp;

    asm("mov %%esp, %0\n"
        "mov %%ebp, %1\n"
        : "=r"(starting_esp), "=r"(starting_ebp)
        :
        :);

    void *stack_base = (void *) starting_esp;
    struct stack_frame *frame = (void *) starting_ebp;

    struct thread_control_block *tcb = __get_self();
    void *stack_end = tcb->attributes.__stack_start + tcb->attributes.__stack_len + tcb->attributes.__guard_size;

    int buffer_index = 0;
    for (; buffer_index < buffer_max; buffer_index++) {
        if (frame == NULL || frame->rip == NULL || frame->next == NULL) {
            break;
        }

        if ((void *) frame < stack_base || (void *) frame > stack_end) {
            break;
        }

        buffer[buffer_index] = frame->rip;
        frame = frame->next;
    }
    return buffer_index;
}
