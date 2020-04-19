#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <unistd.h>

size_t fwrite_unlocked(const void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream) {
    if (stream->__flags & __STDIO_ERROR) {
        return 0;
    }

    if (stream->__flags & __STDIO_LAST_OP_READ) {
        if (fflush_unlocked(stream)) {
            return 0;
        }

        stream->__flags &= ~__STDIO_LAST_OP_READ;
    }

    stream->__flags |= __STDIO_LAST_OP_WRITE;

    size_t to_write = size * nmemb;
    if (to_write == 0) {
        return 0;
    }

    stream->__flags &= ~__STDIO_EOF;

    __stdio_log(stream, "fwrite_unlocked: %p %lu %lu %d", buf, size, nmemb, stream->__fd);
    if (stream->__flags & _IONBF) {
        ssize_t ret = write(stream->__fd, buf, to_write);
        if (ret < 0) {
            stream->__flags |= __STDIO_ERROR;
            return 0;
        }

        return (size_t) ret / size;
    }

    if (stream->__flags & _IOLBF) {
        size_t bytes_written = 0;
        for (; bytes_written < to_write; bytes_written++) {
            if (fputc_unlocked(((const unsigned char *) buf)[bytes_written], stream) == EOF) {
                break;
            }
        }

        return bytes_written / size;
    }

    size_t new_buffer_offset = stream->__position + to_write;
    if (new_buffer_offset < stream->__buffer_max) {
        memcpy(stream->__buffer + stream->__position, buf, to_write);
        stream->__position = new_buffer_offset;
        stream->__buffer_length = MAX(stream->__buffer_length, new_buffer_offset);
        return to_write / size;
    }

    size_t max_written = to_write;
    size_t bytes_to_flush = stream->__position;
    size_t extra_buffer_length = stream->__buffer_max - new_buffer_offset;
    size_t bytes_to_skip_buffering = extra_buffer_length + ((to_write - extra_buffer_length) / stream->__buffer_max) * stream->__buffer_max;
    size_t total_writev_bytes = stream->__position + bytes_to_skip_buffering;

    struct iovec vec[2] = { { .iov_base = stream->__buffer, .iov_len = bytes_to_flush },
                            { .iov_base = (void *) buf, .iov_len = bytes_to_skip_buffering } };
    ssize_t ret = writev(stream->__fd, vec, 2);
    if (ret < (ssize_t) bytes_to_flush) {
        stream->__flags |= __STDIO_ERROR;
        return 0;
    }

    to_write -= bytes_to_skip_buffering;
    if (ret != (ssize_t) total_writev_bytes || to_write == 0) {
        stream->__position = stream->__buffer_length = 0;
        return (ret - bytes_to_flush) / size;
    }

    memcpy(stream->__buffer, (char *) buf + bytes_to_skip_buffering, to_write);
    stream->__position = stream->__buffer_length = to_write;
    return max_written / size;
}

#endif /* OLD_STDIO */