#ifndef OLD_STDIO

#include <bits/lock.h>
#include <errno.h>
#include <fcntl.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>

FILE *freopen(const char *__restrict path, const char *__restrict mode, FILE *__restrict stream) {
    struct __stdio_flags flags = __stdio_parse_mode_string(mode);
    if (flags.__open_flags == -1 && flags.__stream_flags == -1) {
        errno = EINVAL;
        return NULL;
    }

    // We need to take this lock in case freopen fails, and we need to remove the file from the list
    __lock(&__file_list_lock);
    __lock(&stream->__lock);
    __stdio_log(NULL, "freopen: %s %s %d", path, mode, stream->__fd);

    fflush_unlocked(stream);

    stream->__flags &= ~(__STDIO_READABLE | __STDIO_WRITABLE | __STDIO_APPEND | __STDIO_ERROR | __STDIO_EOF);
    if (!path) {
        flags.__open_flags &= ~O_TRUNC;
        if (fcntl(stream->__fd, F_SETFL, flags.__open_flags)) {
            goto freopen_fail_and_close;
        }

        stream->__flags |= flags.__stream_flags;
        goto freopen_success;
    }

    if (close(stream->__fd)) {
        goto freopen_fail;
    }

    int fd = open(path, flags.__open_flags, 0666);
    if (fd < 0) {
        goto freopen_fail;
    }

    stream->__fd = fd;
    stream->__flags |= flags.__stream_flags;

freopen_success:
    __unlock(&stream->__lock);
    __unlock(&__file_list_lock);
    return stream;

freopen_fail_and_close:
    close(stream->__fd);

freopen_fail:
    if (__file_list_tail == stream) {
        __file_list_tail = stream->__prev;
    }
    if (__file_list_head == stream) {
        __file_list_head = stream->__next;
    }
    remque(stream);
    __unlock(&__file_list_lock);

    if (stream->__flags & __STDIO_OWNED) {
        free(stream->__buffer);
    }

    __unlock(&stream->__lock);

    if (stream->__flags & __STDIO_DYNAMICALLY_ALLOCATED) {
        free(stream);
    }
    return NULL;
}

#endif /* OLD_STDIO */
