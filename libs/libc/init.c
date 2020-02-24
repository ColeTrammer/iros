#define __libc_internal

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

__thread int errno;
char **environ;

struct initial_process_info __initial_process_info;
struct thread_control_block *__threads;

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

    sigset_t set = { 0 };
    set |= (UINT64_C(1) << (__PTHREAD_CANCEL_SIGNAL - UINT64_C(1)));
    syscall(SC_SIGPROCMASK, SIG_BLOCK, &set, NULL);

    environ = envp;

    init_env();
    init_files(__initial_process_info.isatty_mask);
}