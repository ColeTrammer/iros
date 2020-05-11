#ifndef OLD_STDIO

#include <stdio.h>

void clearerr_unlocked(FILE *stream) {
    stream->__flags &= ~(__STDIO_ERROR | __STDIO_EOF);
}

#endif /* OLD_STDIO */
