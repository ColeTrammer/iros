#include <stdio.h>

int ferror_unlocked(FILE *stream) {
    return stream->__flags & __STDIO_ERROR;
}
