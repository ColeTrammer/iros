#include <stdio.h>

int putc_unlocked(int c, FILE *stream) {
    return fputc_unlocked(c, stream);
}
