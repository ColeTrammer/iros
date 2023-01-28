#include <bits/lock.h>
#include <stdio.h>

void rewind(FILE *stream) {
    __lock_recursive(&stream->__lock);
    fseek_unlocked(stream, 0, SEEK_SET);
    clearerr_unlocked(stream);
    __unlock_recursive(&stream->__lock);
}
