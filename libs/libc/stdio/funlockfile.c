#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

void funlockfile(FILE *stream) {
    __unlock(&stream->__lock);
}

#endif /* OLD_STDIO */