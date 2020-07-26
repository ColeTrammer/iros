#define __libc_internal

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
    size_t align = MAX(__initial_process_info->tls_alignment, alignof(struct thread_control_block));
    size_t size = ALIGN_UP(__initial_process_info->tls_size, align) + sizeof(struct thread_control_block);
    uint8_t *pointer = aligned_alloc(align, size);
    struct thread_control_block *block = (struct thread_control_block *) (pointer + ALIGN_UP(__initial_process_info->tls_size, align));

    uint8_t *tls = ((uint8_t *) block) - ALIGN_UP(__initial_process_info->tls_size, __initial_process_info->tls_alignment);
    memcpy(tls, __initial_process_info->tls_start, __initial_process_info->tls_size);

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
