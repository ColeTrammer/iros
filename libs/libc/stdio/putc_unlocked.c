#ifdef NEW_STDIO

#include <stdio.h>

int putc_unlocked(int c, FILE *stream) {
    return fputc_unlocked(c, stream);
}

#endif /* NEW_STDIO */