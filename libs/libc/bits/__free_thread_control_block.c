#define __libc_internal

#include <pthread.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/os_2.h>
#include <sys/param.h>

void __free_thread_control_block(struct thread_control_block *block) {
    free(block->pthread_specific_data);
    uint8_t *block_start = (uint8_t *) block;

    block_start -=
        ALIGN_UP(__initial_process_info.tls_size, MAX(__initial_process_info.tls_alignment, alignof(struct thread_control_block)));

    free(block_start);
}