#include <sys/os_2.h>
#include <sys/syscall.h>

__attribute__((__noreturn__)) void exit_task(void) {
    syscall(SYS_exit_task);
    __builtin_unreachable();
}
