#ifdef NEW_STDIO

#include <bits/lock.h>
#include <fcntl.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct __stdio_flags __stdio_parse_mode_string(const char *mode) {
    int open_flags;
    int stream_flags;
    if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0) {
        open_flags = O_RDONLY;
        stream_flags = __STDIO_READABLE;
    } else if (strcmp(mode, "w") == 0 || strcmp(mode, "wb") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_TRUNC;
        stream_flags = __STDIO_WRITABLE;
    } else if (strcmp(mode, "a") == 0 || strcmp(mode, "ab") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_APPEND;
        stream_flags = __STDIO_APPEND | __STDIO_WRITABLE;
    } else if (strcmp(mode, "r+") == 0 || strcmp(mode, "rb+") == 0 || strcmp(mode, "r+b") == 0) {
        open_flags = O_RDWR;
        stream_flags = __STDIO_READABLE | __STDIO_WRITABLE;
    } else if (strcmp(mode, "w+") == 0 || strcmp(mode, "wb+") == 0 || strcmp(mode, "w+b") == 0) {
        open_flags = O_RDWR | O_CREAT | O_TRUNC;
        stream_flags = __STDIO_READABLE | __STDIO_WRITABLE;
    } else if (strcmp(mode, "a+") == 0 || strcmp(mode, "ab+") == 0 || strcmp(mode, "a+b") == 0) {
        open_flags = O_RDWR | O_CREAT | O_APPEND;
        stream_flags = __STDIO_READABLE | __STDIO_WRITABLE | __STDIO_APPEND;
    } else {
        open_flags = -1;
        stream_flags = -1;
    }

    return (struct __stdio_flags) { .__stream_flags = stream_flags, .__open_flags = open_flags };
}

FILE *__stdio_allocate_stream(int fd, int flags) {
    FILE *stream = malloc(sizeof(FILE));
    if (!stream) {
        return NULL;
    }

    stream->__buffer = malloc(BUFSIZ);
    if (!stream->__buffer) {
        free(stream);
        return NULL;
    }

    stream->__buffer_max = BUFSIZ;
    stream->__buffer_length = 0;
    stream->__position = 0;
    stream->__fd = fd;
    stream->__flags = flags | _IOFBF | __STDIO_DYNAMICALLY_ALLOCATED | __STDIO_OWNED;
    stream->__lock = 0;
    stream->__ungetc_character = 0;

    __lock(&__file_list_lock);
    insque(stream, __file_list_tail);
    if (!__file_list_head) {
        __file_list_head = __file_list_tail = stream;
    } else {
        __file_list_tail = stream;
    }
    __unlock(&__file_list_lock);

    return stream;
}

#endif /* NEW_STDIO */