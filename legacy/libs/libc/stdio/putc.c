#include <bits/lock.h>
#include <stdio.h>

int putc(int c, FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = putc_unlocked(c, stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
