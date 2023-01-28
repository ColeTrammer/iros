#include <bits/lock.h>
#include <stdio.h>

int getc(FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = getc_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
