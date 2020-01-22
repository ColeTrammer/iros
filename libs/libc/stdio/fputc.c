#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int fputc(int c, FILE *stream) {
    __lock(&stream->__lock);
    int ret = fputc_unlocked(c, stream);
    __unlock(&stream->__lock);
    return ret;
}

#endif /* OLD_STDIO */