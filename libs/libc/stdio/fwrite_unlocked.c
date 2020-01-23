#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <unistd.h>

size_t fwrite_unlocked(const void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream) {
    size_t to_write = size * nmemb;
    if (to_write == 0) {
        return 0;
    }

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

    size_t max_written = to_write;
    size_t new_buffer_offset = stream->__position + to_write;
    if (new_buffer_offset < stream->__buffer_max) {
        memcpy(stream->__buffer + stream->__position, buf, to_write);
        stream->__position = new_buffer_offset;
        stream->__buffer_length = MAX(stream->__buffer_length, new_buffer_offset);
        return to_write / size;
    }

    size_t extra_buffer_length = stream->__buffer_max - new_buffer_offset;
    to_write -= extra_buffer_length;
    size_t bytes_to_skip_buffering = (to_write / stream->__buffer_max) * stream->__buffer_max;
    size_t total_writev_bytes = extra_buffer_length + bytes_to_skip_buffering;

    struct iovec vec[2] = { { .iov_base = stream->__buffer, .iov_len = stream->__position },
                            { .iov_base = (void *) buf, .iov_len = total_writev_bytes } };
    ssize_t ret = writev(stream->__fd, vec, 2);
    if (ret < 0) {
        stream->__flags |= __STDIO_ERROR;
        return 0;
    }

    to_write -= total_writev_bytes;
    if (ret != (ssize_t) total_writev_bytes || to_write == 0) {
        stream->__position = stream->__buffer_length = 0;
        return total_writev_bytes / size;
    }

    memcpy(stream->__buffer, buf, to_write);
    stream->__position = stream->__buffer_length = to_write;
    return max_written / size;
}

#endif /* OLD_STDIO */