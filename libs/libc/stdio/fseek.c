#include <bits/lock.h>
#include <stdio.h>

int fseek(FILE *stream, long offset, int whence) {
    __lock_recursive(&stream->__lock);
    int ret = fseek_unlocked(stream, offset, whence);
    __unlock_recursive(&stream->__lock);
    return ret;
}
