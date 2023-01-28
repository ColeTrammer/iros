#include <stdio.h>

int fileno_unlocked(FILE *stream) {
    return stream->__fd;
}
