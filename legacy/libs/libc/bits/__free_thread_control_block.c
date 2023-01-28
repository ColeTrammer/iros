#define __libc_internal

#include <bits/tls_record.h>
#include <pthread.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/iros.h>
#include <sys/param.h>

void __free_thread_control_block(struct thread_control_block *block) {
    free(block->pthread_specific_data);

#ifdef __is_shared
    size_t initial_count = __loader_tls_initial_record_count();
    size_t total_count = __loader_tls_num_records();
    for (size_t i = initial_count + 1; i <= total_count; i++) {
        void *allocated_tls = block->dynamic_thread_vector[i].pointer;
        if (allocated_tls) {
            free(allocated_tls);
        }
    }

    free(block->dynamic_thread_vector);
#endif /* __is_shared */

#ifdef __is_static
    size_t tls_alignment = __initial_process_info->tls_alignment;
    size_t tls_size = __initial_process_info->tls_size;
#else
    size_t tls_alignment = __loader_tls_initial_image_alignment();
    size_t tls_size = __loader_tls_initial_image_size();
#endif /* __is_static */

    size_t align = MAX(tls_alignment, alignof(struct thread_control_block));

    uint8_t *block_start = (uint8_t *) block;
    block_start -= ALIGN_UP(tls_size, align);
    free(block_start);
}
