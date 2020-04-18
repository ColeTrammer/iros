#ifndef OLD_STDIO

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int fseek_unlocked(FILE *stream, long offset, int whence) {
    __stdio_log(stream, "fseek: %d, %ld, %d", stream->__fd, offset, whence);

    if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
        stream->__flags &= ~__STDIO_HAS_UNGETC_CHARACTER;
        offset--;
    }

    if (whence == SEEK_CUR) {
        off_t new_position = stream->__position + offset;
        if (new_position >= 0 && new_position < (off_t) stream->__buffer_max) {
            if (new_position > (off_t) stream->__buffer_length) {
                memset(stream->__buffer + stream->__buffer_length, 0, new_position - stream->__buffer_length);
                stream->__buffer_length = new_position;
            }
            stream->__position = new_position;
            goto fseek_end;
        }
    }

    if (fflush_unlocked(stream) == EOF) {
        return -1;
    }

    if (lseek(stream->__fd, offset, whence) < 0) {
        return -1;
    }

fseek_end:
    stream->__flags &= ~__STDIO_EOF;
    return 0;
}

#endif /* OLD_STDIO */