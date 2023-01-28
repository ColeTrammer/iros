#include <stdio.h>

int fseek_unlocked(FILE *stream, long offset, int whence) {
    return fseeko_unlocked(stream, offset, whence);
}
