#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

int getc(FILE *stream) {
    __lock(&stream->__lock);
    int ret = getc(stream);
    __unlock(&stream->__lock);
    return ret;
}

#endif /* NEW_STDIO */