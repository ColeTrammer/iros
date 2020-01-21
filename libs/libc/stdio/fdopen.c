#ifdef NEW_STDIO

#include <bits/lock.h>
#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdio.h>

FILE *fdopen(int fd, const char *mode) {
    struct __stdio_flags flags = __stdio_parse_mode_string(mode);
    if (flags.__open_flags == -1 && flags.__stream_flags == -1) {
        errno = EINVAL;
        return NULL;
    }

    flags.__open_flags &= ~O_TRUNC;
    int ret = fcntl(fd, F_SETFL, flags.__open_flags);
    if (ret < 0) {
        return NULL;
    }

    return __stdio_allocate_stream(fd, flags.__stream_flags);
}

#endif /* NEW_STDIO */