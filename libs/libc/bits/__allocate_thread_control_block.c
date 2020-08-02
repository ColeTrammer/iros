#define __libc_internal

#include <bits/tls_record.h>
#include <limits.h>
#include <pthread.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/os_2.h>
#include <sys/param.h>

struct thread_control_block *__allocate_thread_control_block() {
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
    for (size_t i = 0; i < tls_record_count; i++) {
        struct tls_record *record = __loader_tls_record_at(i);
        if (!record) {
            continue;
        }
        uint8_t *tls = ((uint8_t *) block) - record->tls_offset;
        memcpy(tls, record->tls_image, record->tls_size);
    }
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
