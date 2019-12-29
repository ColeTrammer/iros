#define __libc_internal

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/os_2.h>
#include <sys/param.h>
#include <sys/syscall.h>
#include <unistd.h>

// clang-format off
#define ALIGN_UP(size, align) ((size_t) ((align) == 0 ? (size) : ((((size) + (align) - 1) / (align)) * (align))))
// clang-format on

__thread int errno = 0;
char **environ = NULL;

struct initial_process_info __initial_process_info;
struct thread_control_block *__threads;

// This is a ridiculous place to put these functions, but it is unclear
// where else they should go
struct thread_control_block *__allocate_thread_control_block() {
    size_t align = MAX(__initial_process_info.tls_alignment, alignof(struct thread_control_block));
    size_t size = ALIGN_UP(__initial_process_info.tls_size, align) + sizeof(struct thread_control_block);
    uint8_t *pointer = aligned_alloc(align, size);
    struct thread_control_block *block = (struct thread_control_block *) (pointer + ALIGN_UP(__initial_process_info.tls_size, align));

    uint8_t *tls = ((uint8_t *) block) - ALIGN_UP(__initial_process_info.tls_size, __initial_process_info.tls_alignment);
    memcpy(tls, __initial_process_info.tls_start, __initial_process_info.tls_size);

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

void __free_thread_control_block(struct thread_control_block *block) {
    free(block->pthread_specific_data);
    uint8_t *block_start = (uint8_t *) block;

    block_start -=
        ALIGN_UP(__initial_process_info.tls_size, MAX(__initial_process_info.tls_alignment, alignof(struct thread_control_block)));

    free(block_start);
}

void initialize_standard_library(int argc, char *argv[], char *envp[]) {
    (void) argc;
    (void) argv;

    get_initial_process_info(&__initial_process_info);

    // FIXME: __allocate_thread_control_block will call malloc which calls sbrk,
    //        which could set errno if it fails.
    //        This should be avoided as tls isn't enabled yet
    __threads = __allocate_thread_control_block();
    __threads->attributes.__stack_start = __initial_process_info.stack_start;
    __threads->attributes.__stack_len = __initial_process_info.stack_size;
    __threads->attributes.__sched_param.sched_priority = 0;
    __threads->attributes.__guard_size = __initial_process_info.guard_size;
    __threads->attributes.__flags = PTHREAD_CREATE_JOINABLE | PTHREAD_INHERIT_SCHED | SCHED_OTHER;
    __threads->id = __initial_process_info.main_tid;

    set_thread_self_pointer(__threads, &__threads->locked_robust_mutex_node_list_head);

    // Don't use wrappers so that signal.o won't be linked in
    sigset_t set = { 0 };
    set |= (1 << __PTHREAD_CANCEL_SIGNAL);
    syscall(SC_SIGPROCMASK, SIG_BLOCK, &set, NULL);

    environ = envp;

    init_files();
    init_env();
}