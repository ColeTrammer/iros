#define __libc_internal

#include <pthread.h>

struct thread_control_block *__get_self() {
    struct thread_control_block *ret;
    asm("movl %%gs:0, %0" : "=r"(ret)::);
    return ret;
}
