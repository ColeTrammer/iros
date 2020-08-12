#include <bits/lock.h>
#include <stdio.h>

void clearerr(FILE *stream) {
    __lock_recursive(&stream->__lock);
    clearerr_unlocked(stream);
    __unlock_recursive(&stream->__lock);
}
