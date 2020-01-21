#ifdef NEW_STDIO

#include <stdio.h>

int fileno(FILE *stream) {
    return stream->__fd;
}

#endif /* NEW_STDIO */