#include <stdio.h>

long ftell_unlocked(FILE *stream) {
    return ftello_unlocked(stream);
}
