#define __libc_internal

#include <bits/tls_record.h>
#include <limits.h>
#include <pthread.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/iros.h>
#include <sys/param.h>

// Reserve some space in the DTV to prevent reallocations.
#define TLS_DTV_RESERVE 5

struct thread_control_block *__allocate_thread_control_block(void) {
#ifdef __is_static
    size_t tls_alignment = __initial_process_info->tls_alignment;
    size_t tls_size = __initial_process_info->tls_size;
#else
    size_t tls_alignment = __loader_tls_initial_image_alignment();
    size_t tls_size = __loader_tls_initial_image_size();
#endif /* __is_static */

    size_t align = MAX(tls_alignment, alignof(struct thread_control_block));
    size_t tls_size_with_padding = ALIGN_UP(tls_size, align);
    size_t size = tls_size_with_padding + sizeof(struct thread_control_block);
    void *pointer = aligned_alloc(align, size);
    struct thread_control_block *block = pointer + tls_size_with_padding;

#ifdef __is_static
    uint8_t *tls = ((uint8_t *) block) - ALIGN_UP(tls_size, tls_alignment);
    memcpy(tls, __initial_process_info->tls_start, tls_size);
#else
    size_t tls_record_count = __loader_tls_num_records();

    size_t dtv_max = tls_record_count + 1 + TLS_DTV_RESERVE;
    block->dynamic_thread_vector = malloc(dtv_max * sizeof(block->dynamic_thread_vector[0]));
    block->dynamic_thread_vector_max = dtv_max;
    block->dynamic_thread_vector[0].number = atomic_load_explicit(&__loader_tls_generation_number, memory_order_relaxed);

    size_t initial_count = __loader_tls_initial_record_count();
    struct tls_record *tls_records = __loader_tls_records;
    for (size_t i = 1; i <= initial_count; i++) {
        struct tls_record *record = &tls_records[i - 1];
        if (!record->tls_image) {
            continue;
        }
        uint8_t *tls = ((uint8_t *) block) - record->tls_offset;
        memcpy(tls, record->tls_image, record->tls_size);
        block->dynamic_thread_vector[i].pointer = tls;
    }
    memset(&block->dynamic_thread_vector[initial_count + 1], 0, sizeof(block->dynamic_thread_vector[0]) * (dtv_max - initial_count - 1));
#endif /* __is_static */

    block->self = block;
    block->next = NULL;
    block->prev = NULL;
    block->exit_value = NULL;
    block->has_exited = false;
    block->concurrency = 0;
    block->joining_thread = 0;
    block->locked_robust_mutex_node_list_head = NULL;
    block->pthread_specific_data = calloc(PTHREAD_KEYS_MAX, sizeof(void *));
    return block;
}
