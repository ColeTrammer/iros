#include <bits/lock.h>
#include <stdio.h>

int fflush(FILE *stream) {
    if (!stream) {
        return fflush_unlocked(NULL);
    }

    __lock_recursive(&stream->__lock);
    int ret = fflush_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
