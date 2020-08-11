#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>

int fcntl(int fd, int command, ...) {
    int ret;
    va_list args;
    va_start(args, command);
    int arg = command == F_GETFD || command == F_GETFL ? 0 : va_arg(args, int);
    ret = (int) syscall(SYS_FCNTL, fd, command, arg);
    va_end(args);
    __SYSCALL_TO_ERRNO(ret);
}
