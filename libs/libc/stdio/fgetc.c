#include <bits/lock.h>
#include <stdio.h>

int fgetc(FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = fgetc_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
