#include <sys/os_2.h>
#include <sys/syscall.h>

__attribute__((__noreturn__)) void exit_task(void) {
    syscall(SC_EXIT_TASK);
    __builtin_unreachable();
}