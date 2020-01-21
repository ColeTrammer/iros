#ifdef NEW_STDIO

#include <stdio.h>

int getc_unlocked(FILE *stream) {
    return fgetc_unlocked(stream);
}

#endif /* NEW_STDIO */