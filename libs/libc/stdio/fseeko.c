#include <bits/lock.h>
#include <stdio.h>

int fseeko(FILE *stream, off_t offset, int whence) {
    __lock_recursive(&stream->__lock);
    int ret = fseek_unlocked(stream, offset, whence);
    __unlock_recursive(&stream->__lock);
    return ret;
}
