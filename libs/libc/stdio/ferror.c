#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int ferror(FILE *stream) {
    __lock(&stream->__lock);
    int ret = ferror_unlocked(stream);
    __unlock(&stream->__lock);
    return ret;
}

#endif /* OLD_STDIO */
