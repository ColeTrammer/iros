#include <bits/lock.h>
#include <stdio.h>

int feof(FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = feof_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
