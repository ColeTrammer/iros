#include <sys/os_2.h>
#include <sys/syscall.h>

int create_task(struct create_task_args *create_task_args) {
    return syscall(SC_CREATE_TASK, create_task_args);
}

__attribute__((__noreturn__)) void exit_task(void) {
    syscall(SC_EXIT_TASK);
    __builtin_unreachable();
}

int get_initial_process_info(struct initial_process_info *info) {
    return syscall(SC_GET_INITIAL_PROCESS_INFO, info);
}

int os_mutex(int *__protected, int op, int expected, int to_place, int to_wake, int *to_wait) {
    return syscall(SC_OS_MUTEX, __protected, op, expected, to_place, to_wake, to_wait);
}

int set_thread_self_pointer(void *p) {
    return syscall(SC_SET_THREAD_SELF_POINTER, p);
}

int tgkill(int tgid, int tid, int signum) {
    return syscall(SC_TGKILL, tgid, tid, signum);
}