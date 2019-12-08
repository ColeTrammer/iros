#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>

int open(const char *pathname, int flags, ...) {
    va_list parameters;
    va_start(parameters, flags);
    mode_t mode = flags & O_CREAT ? va_arg(parameters, mode_t) : 0;
    int ret = (int) syscall(SC_OPEN, pathname, flags, mode);
    va_end(parameters);
    __SYSCALL_TO_ERRNO(ret);
}

int creat(const char *pathname, mode_t mode) {
    return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int fcntl(int fd, int command, ...) {
    int ret;
    va_list args;
    va_start(args, command);
    int arg = command == F_GETFD || command == F_GETFL ? 0 : va_arg(args, int);
    ret = (int) syscall(SC_FCNTL, fd, command, arg);
    va_end(args);
    __SYSCALL_TO_ERRNO(ret);
}