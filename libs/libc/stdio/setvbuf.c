#include <bits/lock.h>
#include <stdio.h>
#include <stdlib.h>

int setvbuf(FILE *__restrict stream, char *__restrict buf, int mode, size_t size) {
    if (mode != _IONBF && mode != _IOLBF && mode != _IOFBF) {
        return -1;
    }

    int ret = 0;
    __lock(&stream->__lock);
    __stdio_log(stream, "setvbuf: [ %d, %p, %d, %lu ]\n", stream->__fd, buf, mode, size);

    fflush_unlocked(stream);

    // Check case where only the buffer mode is being changed.
    if ((stream->__flags & __STDIO_OWNED) && (buf == NULL) && (size == stream->__buffer_max)) {
        goto update_flags;
    }

    if (stream->__flags & __STDIO_OWNED) {
        free(stream->__buffer);
        stream->__flags &= ~__STDIO_OWNED;
        stream->__buffer = NULL;
        stream->__buffer_max = 0;
    }

    if (buf == NULL) {
        buf = malloc(size);
        if (!buf) {
            ret = -1;
            mode = _IONBF; /* Set mode to _IONBF since we no longer have one */
            goto update_flags;
        }
        stream->__flags |= __STDIO_OWNED;
    }

    stream->__buffer = buf;
    stream->__buffer_max = size;

update_flags:
    stream->__flags &= ~(_IOFBF | _IONBF | _IOLBF);
    stream->__flags |= mode;
    __unlock(&stream->__lock);
    return ret;
}
