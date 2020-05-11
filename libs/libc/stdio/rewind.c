#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

void rewind(FILE *stream) {
    __lock(&stream->__lock);
    fseek_unlocked(stream, 0, SEEK_SET);
    clearerr_unlocked(stream);
    __unlock(&stream->__lock);
}

#endif /* OLD_STDIO */
