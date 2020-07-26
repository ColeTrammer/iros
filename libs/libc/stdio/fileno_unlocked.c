#ifndef OLD_STDIO

#include <stdio.h>

int fileno_unlocked(FILE *stream) {
    return stream->__fd;
}

#endif /* OLD_STDIO */
