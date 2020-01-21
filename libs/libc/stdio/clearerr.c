#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

void clearerr(FILE *stream) {
    __lock(&stream->__lock);
    clearerr_unlocked(stream);
    __unlock(&stream->__lock);
}

#endif /* NEW_STDIO */