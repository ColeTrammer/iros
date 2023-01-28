#include <bits/lock.h>
#include <stdio.h>

int fputc(int c, FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = fputc_unlocked(c, stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
