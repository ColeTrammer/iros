#ifndef OLD_STDIO

#include <stdio.h>
#include <unistd.h>

int fputc_unlocked(int a, FILE *stream) {
    unsigned char c = (unsigned char) a;
    if (stream->__flags & _IONBF) {
        ssize_t ret = write(stream->__fd, &c, 1);
        if (ret != 1) {
            stream->__flags |= __STDIO_ERROR;
            return EOF;
        }

        return (int) c;
    }

    if (stream->__position >= (off_t) stream->__buffer_max || ((stream->__flags & _IOLBF) && c == '\n')) {
        if (fflush_unlocked(stream) == EOF) {
            return EOF;
        }
    }

    if (stream->__position == (off_t) stream->__buffer_length) {
        stream->__buffer_length++;
    }

    stream->__buffer[stream->__position++] = c;
    return (int) c;
}

#endif /* OLD_STDIO */