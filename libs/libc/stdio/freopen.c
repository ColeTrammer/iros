#ifdef NEW_STDIO

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

FILE *freopen(const char *__restrict path, const char *__restrict mode, FILE *__restrict stream) {
    fflush(stream);

    struct __stdio_flags flags = __stdio_parse_mode_string(mode);
    if (flags.__open_flags == -1 && flags.__stream_flags == -1) {
        errno = EINVAL;
        return NULL;
    }

    stream->__flags &= ~(__STDIO_READABLE | __STDIO_WRITABLE | __STDIO_APPEND | __STDIO_ERROR | __STDIO_EOF);
    if (!path) {
        flags.__open_flags &= ~O_TRUNC;
        if (fcntl(stream->__fd, F_SETFL, flags.__open_flags)) {
            return NULL;
        }

        stream->__flags |= flags.__stream_flags;
        return stream;
    }

    int fd = open(path, flags.__open_flags, 0666);
    if (fd < 0) {
        return NULL;
    }

    stream->__fd = fd;
    stream->__flags |= flags.__stream_flags;
    return stream;
}

#endif /* NEW_STDIO */