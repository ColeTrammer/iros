#include <fcntl.h>
#include <stdarg.h>

int open(const char *pathname, int flags, ...) {
    va_list parameters;
    va_start(parameters, flags);
    int ret;
    if (flags & O_CREAT) {
        ret = openat(AT_FDCWD, pathname, flags, va_arg(parameters, mode_t));
    } else {
        ret = openat(AT_FDCWD, pathname, flags);
    }
    va_end(parameters);
    return ret;
}