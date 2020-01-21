#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

int putc(int c, FILE *stream) {
    __lock(&stream->__lock);
    int ret = putc_unlocked(c, stream);
    __unlock(&stream->__lock);
    return ret;
}

#endif /* NEW_STDIO */