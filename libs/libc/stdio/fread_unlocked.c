#ifndef OLD_STDIO

#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

size_t fread_unlocked(void *__restrict buf, size_t size, size_t nmemb, FILE *__restrict stream) {
    __stdio_log(stream, "fread_unlocked: %p %lu %lu %d", buf, size, nmemb, stream->__fd);

    unsigned char *buffer = (unsigned char *) buf;

    size_t to_read = size * nmemb;
    size_t bytes_read = 0;
    if (to_read != 0 && (stream->__flags & __STDIO_HAS_UNGETC_CHARACTER)) {
        stream->__flags &= ~__STDIO_HAS_UNGETC_CHARACTER;
        buffer[bytes_read++] = stream->__ungetc_character;
        to_read--;
    }

    if (to_read == 0) {
        return bytes_read / size;
    }

    if (stream->__flags & _IONBF) {
        ssize_t ret = read(stream->__fd, buffer + bytes_read, to_read);
        if (ret < 0) {
            stream->__flags |= __STDIO_ERROR;
        } else if ((size_t) ret < to_read) {
            stream->__flags |= __STDIO_EOF;
            bytes_read += (size_t) ret;
        } else {
            bytes_read += to_read;
        }

        return bytes_read / size;
    }

    if (stream->__flags & _IOLBF) {
        for (; bytes_read < to_read;) {
            int c = fgetc_unlocked(stream);
            if (c == EOF) {
                break;
            }

            buffer[bytes_read++] = (unsigned char) c;
        }

        return bytes_read / size;
    }

    size_t can_read_from_current_buffer = stream->__buffer_length - stream->__position;
    if (to_read <= can_read_from_current_buffer) {
        memcpy(buffer + bytes_read, stream->__buffer + stream->__position, to_read);
        stream->__position += to_read;
        bytes_read += to_read;
        return bytes_read / size;
    } else if (stream->__buffer_length != 0 && stream->__buffer_length < stream->__buffer_max) {
        stream->__flags |= __STDIO_EOF;
        memcpy(buffer + bytes_read, stream->__buffer + stream->__position, can_read_from_current_buffer);
        stream->__position = stream->__buffer_length;
        bytes_read += can_read_from_current_buffer;
        return bytes_read / size;
    }

    size_t need_to_read_with_syscall = to_read - can_read_from_current_buffer;
    size_t bytes_to_skip_buffering = (need_to_read_with_syscall / stream->__buffer_max) * stream->__buffer_max;
    size_t remaining_after_read = need_to_read_with_syscall - bytes_to_skip_buffering;

    memcpy(buffer + bytes_read, stream->__buffer + stream->__position, can_read_from_current_buffer);
    bytes_read += can_read_from_current_buffer;

    stream->__position = stream->__buffer_length = 0;
    struct iovec vec[2] = { { .iov_base = buffer + bytes_read, .iov_len = bytes_to_skip_buffering },
                            { .iov_base = stream->__buffer, .iov_len = stream->__buffer_max } };
    ssize_t ret = readv(stream->__fd, vec, 2);
    if (ret < 0) {
        stream->__flags |= __STDIO_ERROR;
        return bytes_read / size;
    } else if ((size_t) ret <= bytes_to_skip_buffering) {
        stream->__flags |= __STDIO_EOF;
        bytes_read += (size_t) ret;
        return bytes_read / size;
    } else {
        bytes_read += (size_t) ret;
    }

    stream->__buffer_length = (size_t) ret - bytes_to_skip_buffering;
    if (stream->__buffer_length >= remaining_after_read) {
        memcpy(buffer + bytes_read, stream->__buffer, remaining_after_read);
        stream->__position = remaining_after_read;
        bytes_read += remaining_after_read;
        return bytes_read / size;
    }

    stream->__flags |= __STDIO_EOF;
    memcpy(buffer + bytes_read, stream->__buffer, stream->__buffer_length);
    stream->__position = stream->__buffer_length;
    bytes_read += stream->__buffer_length;
    return bytes_read / size;
}

#endif /* OLD_STDIO */