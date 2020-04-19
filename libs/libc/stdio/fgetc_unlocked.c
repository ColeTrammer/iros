#ifndef OLD_STDIO

#include <stdio.h>
#include <unistd.h>

int fgetc_unlocked(FILE *stream) {
    if ((stream->__flags & __STDIO_EOF) || (stream->__flags & __STDIO_ERROR)) {
        return EOF;
    }

    if (stream->__flags & __STDIO_LAST_OP_WRITE) {
        if (fflush_unlocked(stream)) {
            return EOF;
        }

        stream->__flags &= ~__STDIO_LAST_OP_WRITE;
    }

    stream->__flags |= __STDIO_LAST_OP_READ;

    if (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER) {
        stream->__flags &= ~__STDIO_HAS_UNGETC_CHARACTER;
        return stream->__ungetc_character;
    }

    if ((stream->__flags & _IONBF) || (stream->__flags & _IOLBF)) {
        unsigned char c;
        ssize_t ret = read(stream->__fd, &c, 1);
        if (ret < 0) {
            stream->__flags |= __STDIO_ERROR;
            return EOF;
        } else if (ret == 0) {
            stream->__flags |= __STDIO_EOF;
            return EOF;
        } else {
            return (int) c;
        }
    }

    if (stream->__position < (off_t) stream->__buffer_length) {
        return (int) stream->__buffer[stream->__position++];
    } else if (stream->__position != 0 && stream->__buffer_length < stream->__buffer_max) {
        stream->__flags |= __STDIO_EOF;
        return EOF;
    } else {
        stream->__position = 0;
        ssize_t ret = read(stream->__fd, stream->__buffer, stream->__buffer_max);
        if (ret < 0) {
            stream->__flags |= __STDIO_ERROR;
            return EOF;
        } else if (ret == 0) {
            stream->__flags |= __STDIO_EOF;
            return EOF;
        } else {
            stream->__buffer_length = ret;
            return (int) stream->__buffer[stream->__position++];
        }
    }
}

#endif /* OLD_STDIO */