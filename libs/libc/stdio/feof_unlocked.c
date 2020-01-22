#ifndef OLD_STDIO

#include <stdio.h>

int feof_unlocked(FILE *stream) {
    return stream->__flags & __STDIO_EOF;
}

#endif /* OLD_STDIO */