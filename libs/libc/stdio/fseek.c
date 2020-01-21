#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int fseek(FILE *stream, long offset, int whence) {
    int ret = 0;
    __lock(&stream->__lock);

    if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
        stream->__flags &= ~__STDIO_HAS_UNGETC_CHARACTER;
        offset--;
    }

    if (whence == SEEK_CUR) {
        off_t new_position = stream->__position + offset;
        if (new_position < 0 || new_position >= (off_t) stream->__buffer_max) {
            offset = new_position < 0 ? -new_position : new_position - stream->__position;
        } else {
            if (new_position > (off_t) stream->__buffer_length) {
                memset(stream->__buffer + stream->__buffer_length, 0, new_position - stream->__buffer_length);
                stream->__buffer_length = new_position;
            }
            stream->__position = new_position;
            goto fseek_end;
        }
    }

    if (fflush_unlocked(stream) == EOF) {
        ret = -1;
        goto fseek_end;
    }

    if (lseek(stream->__fd, offset, whence)) {
        ret = -1;
    }

fseek_end:
    if (ret == 0) {
        stream->__flags &= ~__STDIO_EOF;
    }

    __unlock(&stream->__lock);
    return ret;
}

#endif /* NEW_STDIO */