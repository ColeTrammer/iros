#include <bits/lock.h>
#include <stdio.h>
#include <stdlib.h>

int setvbuf(FILE *__restrict stream, char *__restrict buf, int mode, size_t size) {
    if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
        return -1;
    }

    __stdio_log(stream, "setvbuf: [ %d, %p, %d, %lu ]\n", stream->__fd, buf, mode, size);

    fflush(stream);

    if (stream->__flags & __STDIO_OWNED) {
        free(stream->__buffer);
    }

    stream->__flags &= ~__STDIO_OWNED;
    if (buf == NULL) {
        buf = malloc(size);
        if (!buf) {
            return -1;
        }
        stream->__flags |= __STDIO_OWNED;
    }

    stream->__buffer = buf;
    stream->__buffer_max = size;
    stream->__flags &= ~(_IOFBF | _IONBF | _IOLBF);
    stream->__flags |= mode;
    return 0;
}
