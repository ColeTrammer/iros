#ifndef OLD_STDIO

#include <stdio.h>

int getc_unlocked(FILE *stream) {
    return fgetc_unlocked(stream);
}

#endif /* OLD_STDIO */
