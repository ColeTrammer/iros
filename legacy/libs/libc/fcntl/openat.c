#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>

int openat(int dirfd, const char *pathname, int flags, ...) {
    va_list parameters;
    va_start(parameters, flags);
    mode_t mode = flags & O_CREAT ? va_arg(parameters, mode_t) : 0;
    int ret = (int) syscall(SYS_openat, dirfd, pathname, flags, mode);
    va_end(parameters);
    __SYSCALL_TO_ERRNO(ret);
}
