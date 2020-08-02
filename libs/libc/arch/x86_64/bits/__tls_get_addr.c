#ifdef __is_shared

#define __libc_internal

#include <bits/tls_record.h>
#include <pthread.h>

typedef struct {
    unsigned long ti_module;
    unsigned long ti_offset;
} tls_index;

void *__tls_get_addr(tls_index *ti) {
    struct thread_control_block *self = __get_self();
    return self->dynamic_thread_vector[ti->ti_module] + ti->ti_offset;
}

#endif /* __is_shared */
