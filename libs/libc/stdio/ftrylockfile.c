#ifndef OLD_STDIO

#include <bits/lock.h>
#include <stdio.h>

int ftrylockfile(FILE *stream) {
    return !__trylock(&stream->__lock);
}

#endif /* OLD_STDIO */
