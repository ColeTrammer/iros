#ifdef NEW_STDIO

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

FILE *fopen(const char *__restrict path, const char *__restrict mode) {
    struct __stdio_flags flags = __stdio_parse_mode_string(mode);
    if (flags.__open_flags == -1 && flags.__stream_flags == -1) {
        errno = EINVAL;
        return NULL;
    }

    int fd = open(path, flags.__open_flags, 0666);
    if (fd < 0) {
        return NULL;
    }

    return __stdio_allocate_stream(fd, flags.__stream_flags);
}

#endif /* NEW_STDIO */