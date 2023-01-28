#ifdef __is_shared

#define __libc_internal

#include <assert.h>
#include <bits/tls_record.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

typedef struct {
    unsigned long ti_module;
    unsigned long ti_offset;
} tls_index;

static void reallocate_dtv(struct thread_control_block *self, size_t new_generation_number) {
    size_t initial_count = __loader_tls_initial_record_count();
    size_t record_count = __loader_tls_initial_record_count();

    size_t old_dtv_max = self->dynamic_thread_vector_max;
    size_t new_dtv_max = record_count + 1;

    size_t our_generation_number = self->dynamic_thread_vector[0].number;
    for (size_t i = initial_count + 1; i < old_dtv_max && i <= record_count; i++) {
        struct tls_record *record = &__loader_tls_records[i - 1];

        // This means this particular DTV entry is out of date (and could have been reused).
        if (record->tls_generation_number > our_generation_number) {
            void *dtv_entry = self->dynamic_thread_vector[i].pointer;
            if (dtv_entry) {
                free(dtv_entry);
                self->dynamic_thread_vector[i].pointer = NULL;
            }
        }
    }

    if (new_dtv_max >= old_dtv_max) {
        new_dtv_max = MAX(old_dtv_max * 2, new_dtv_max);

        void *new_dtv = realloc(self->dynamic_thread_vector, sizeof(self->dynamic_thread_vector[0]) * new_dtv_max);
        assert(new_dtv);
        self->dynamic_thread_vector = new_dtv;
        memset(&self->dynamic_thread_vector[old_dtv_max], 0, sizeof(self->dynamic_thread_vector[0]) * (new_dtv_max - old_dtv_max));
    }

    self->dynamic_thread_vector_max = new_dtv_max;
    self->dynamic_thread_vector[0].number = new_generation_number;
}

static void *allocate_tls(struct tls_record *record) {
    void *memory = NULL;

    if (record->tls_align > sizeof(void *)) {
        // NOTE: aligned_alloc isn't the best, since it could set errno, which could then call __tls_get_addr().
        //       However, this is fine as long as errno is part of the initial tls image, since it would already be allocated.
        assert(!(record->tls_align & (record->tls_align - 1)));
        memory = aligned_alloc(record->tls_align, record->tls_size);
    } else {
        // Alignment is small enough that malloc() will suffice.
        memory = malloc(record->tls_size);
    }
    assert(memory);
    memcpy(memory, record->tls_image, record->tls_size);
    return memory;
}

__attribute__((__regparm__(1))) void *___tls_get_addr(tls_index *ti) {
    struct thread_control_block *self = __get_self();
    size_t current_gen_number = atomic_load_explicit(&__loader_tls_generation_number, memory_order_relaxed);
    if (current_gen_number != self->dynamic_thread_vector[0].number) {
        reallocate_dtv(self, current_gen_number);
    }

    void *base = self->dynamic_thread_vector[ti->ti_module].pointer;
    if (!base) {
        struct tls_record *record = &__loader_tls_records[ti->ti_module - 1];
        base = self->dynamic_thread_vector[ti->ti_module].pointer = allocate_tls(record);
    }

    return base + ti->ti_offset;
}

#endif /* __is_shared */
