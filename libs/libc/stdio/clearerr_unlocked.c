#ifdef NEW_STDIO

#include <stdio.h>

void clearerr_unlocked(FILE *stream) {
    stream->__flags &= ~__STDIO_ERROR;
}

#endif /* NEW_STDIO */