#include <sys/syscall.h>
#include <unistd.h>

__attribute__((__noreturn__)) void _exit(int status) {
    syscall(SYS_EXIT, status);
    __builtin_unreachable();
}
