#include <stdio.h>

int feof_unlocked(FILE *stream) {
    return stream->__flags & __STDIO_EOF;
}
