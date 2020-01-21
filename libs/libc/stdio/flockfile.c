#ifdef NEW_STDIO

#include <bits/lock.h>
#include <stdio.h>

void flockfile(FILE *stream) {
    __lock(&stream->__lock);
}

#endif /* NEW_STDIO */