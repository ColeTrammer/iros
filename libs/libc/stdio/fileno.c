#include <bits/lock.h>
#include <stdio.h>

int fileno(FILE *stream) {
    __lock(&stream->__lock);
    int ret = fileno_unlocked(stream);
    __unlock(&stream->__lock);
    return ret;
}
