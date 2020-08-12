#include <bits/lock.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int fclose(FILE *stream) {
    // This lock order is important to prevent deadlock
    // when someone calls fflush(NULL)
    __lock(&__file_list_lock);
    __lock_recursive(&stream->__lock);

    __stdio_log(NULL, "fclose: %d", stream->__fd);

    if (__file_list_tail == stream) {
        __file_list_tail = stream->__prev;
    }
    if (__file_list_head == stream) {
        __file_list_head = stream->__next;
    }
    remque(stream);
    __unlock(&__file_list_lock);

    int ret1 = fflush_unlocked(stream);
    int ret2 = close(stream->__fd);

    if (stream->__flags & __STDIO_OWNED) {
        free(stream->__buffer);
    }

    __unlock_recursive(&stream->__lock);

    if (stream->__flags & __STDIO_DYNAMICALLY_ALLOCATED) {
        free(stream);
    }
    return (ret1 == -1 || ret2 == -1) ? -1 : 0;
}
