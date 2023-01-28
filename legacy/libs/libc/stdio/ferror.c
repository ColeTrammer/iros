#include <bits/lock.h>
#include <stdio.h>

int ferror(FILE *stream) {
    __lock_recursive(&stream->__lock);
    int ret = ferror_unlocked(stream);
    __unlock_recursive(&stream->__lock);
    return ret;
}
