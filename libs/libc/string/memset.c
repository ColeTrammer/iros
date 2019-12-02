#include <string.h>

void *memset(void *s, int c, size_t n) {
    unsigned char *buffer = (unsigned char *) s;
    for (size_t i = 0; i < n; i++) {
        buffer[i] = (unsigned char) c;
    }
    return buffer;
}