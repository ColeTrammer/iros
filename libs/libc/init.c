#define __libc_internal

#include <bits/program_name.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/os_2.h>
#include <sys/param.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __is_static
extern void (*__preinit_array_start[])(int, char **, char **);
extern void (*__preinit_array_end[])(int, char **, char **);
extern void (*__init_array_start[])(int, char **, char **);
extern void (*__init_array_end[])(int, char **, char **);

extern void _init(void);
#endif /* __is_static */

__thread int errno;
char **environ;
const char *__program_name = NULL;

struct thread_control_block *__threads;

#ifdef __is_shared
static __attribute__((constructor)) void early_init(int argc, char **argv, char **envp) {
    (void) argc;
    (void) argv;

    environ = envp;

    init_files(__initial_process_info->isatty_mask);
    init_env();
}
#endif /* __is_shared */

void initialize_standard_library(struct initial_process_info *initial_process_info, int argc, char **argv, char **envp) {
    (void) initial_process_info;
    (void) argc;
    (void) argv;
    (void) envp;

#ifdef __is_static
    environ = envp;
    __initial_process_info = initial_process_info;

    init_files(__initial_process_info->isatty_mask);
    init_env();
#endif /* #ifdef __is_static */

    // FIXME: __allocate_thread_control_block will call malloc which calls sbrk,
    //        which could set errno if it fails.
    //        This should be avoided as tls isn't enabled yet
    __threads = __allocate_thread_control_block();
    __threads->attributes.__stack_start = __initial_process_info->stack_start;
    __threads->attributes.__stack_len = __initial_process_info->stack_size;
    __threads->attributes.__sched_param.sched_priority = 0;
    __threads->attributes.__guard_size = __initial_process_info->guard_size;
    __threads->attributes.__flags = PTHREAD_CREATE_JOINABLE | PTHREAD_INHERIT_SCHED | SCHED_OTHER | __PTHREAD_MAUALLY_ALLOCATED_STACK;
    __threads->id = __initial_process_info->main_tid;

    set_thread_self_pointer(__threads, &__threads->locked_robust_mutex_node_list_head);

    sigset_t set = { 0 };
    set |= (UINT64_C(1) << (__PTHREAD_CANCEL_SIGNAL - UINT64_C(1)));
    syscall(SC_SIGPROCMASK, SIG_BLOCK, &set, NULL);

#ifdef __is_static
    const size_t preinit_size = __preinit_array_end - __preinit_array_start;
    for (size_t i = 0; i < preinit_size; i++) {
        (*__preinit_array_start[i])(argc, argv, envp);
    }

    _init();

    const size_t size = __init_array_end - __init_array_start;
    for (size_t i = 0; i < size; i++) {
        (*__init_array_start[i])(argc, argv, envp);
    }
#endif /* __is_static */

    __program_name = strrchr(argv[0], '/');
    if (!__program_name) {
        __program_name = argv[0];
    }
}
