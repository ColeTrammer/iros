#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int fseek(FILE *stream, long offset, int whence) {
    __lock(&stream->__lock);
    int ret = fseek_unlocked(stream, offset, whence);
    __unlock(&stream->__lock);
    return ret;
}

#endif /* OLD_STDIO */