#ifdef NEW_STDIO

#include <stdio.h>

int getc(FILE *stream) {
    return fgetc(stream);
}

#endif /* NEW_STDIO */