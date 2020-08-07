#include <bits/lock.h>
#include <stdio.h>

int getc(FILE *stream) {
    __lock(&stream->__lock);
    int ret = getc_unlocked(stream);
    __unlock(&stream->__lock);
    return ret;
}
