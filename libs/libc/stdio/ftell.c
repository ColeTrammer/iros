#include <stdio.h>

long ftell(FILE *stream) {
    return ftello(stream);
}
