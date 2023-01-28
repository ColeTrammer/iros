#include <bits/lock.h>
#include <stdio.h>

int fileno(FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = fileno_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
