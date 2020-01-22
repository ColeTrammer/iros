#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int fflush(FILE *stream) {
    if (!stream) {
        return fflush_unlocked(NULL);
    }

    __lock(&stream->__lock);
    int ret = fflush_unlocked(stream);
    __unlock(&stream->__lock);
    return ret;
}

#endif /* OLD_STDIO */