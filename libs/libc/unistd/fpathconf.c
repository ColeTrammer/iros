#include <errno.h>
#include <limits.h>
#include <unistd.h>

long fpathconf(int fd, int name) {
    (void) fd;
    switch (name) {
        case _PC_PATH_MAX:
            return PATH_MAX;
        case _PC_PIPE_BUF:
            return PIPE_BUF;
        default:
            errno = EINVAL;
            return -1;
    }
}
