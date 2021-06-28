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
    uint64_t starting_rsp;
    uint64_t starting_rbp;

    asm("mov %%rsp, %0\n"
        "mov %%rbp, %1\n"
        : "=r"(starting_rsp), "=r"(starting_rbp)
        :
        :);

    void *stack_base = (void *) starting_rsp;
    struct stack_frame *frame = (void *) starting_rbp;

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
