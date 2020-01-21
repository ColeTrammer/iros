#ifdef NEW_STDIO

#include <bits/lock.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int fclose(FILE *stream) {
    __lock(&stream->__lock);

    int ret1 = fflush_unlocked(stream);
    int ret2 = close(stream->__fd);

    if (stream->__flags & __STDIO_OWNED) {
        free(stream->__buffer);
    }

    __lock(&__file_list_lock);
    if (__file_list_tail == stream) {
        __file_list_tail = stream->__next;
    }
    if (__file_list_head == stream) {
        __file_list_tail = stream->__prev;
    }
    remque(stream);
    __unlock(&__file_list_lock);

    __unlock(&stream->__lock);

    if (stream->__flags & __STDIO_DYNAMICALLY_ALLOCATED) {
        free(stream);
    }
    return (ret1 == -1 || ret2 == -1) ? -1 : 0;
}

#endif /* NEW_STDIO */