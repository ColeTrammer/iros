#include <string.h>

void *memchr(const void *s, int c, size_t n) {
    const unsigned char *buffer = (const unsigned char*) s;
    unsigned char val = (unsigned char) c;

    for (size_t i = 0; i < n; i++) {
        if (buffer[i] == val) {
            return (char*) buffer + i;
        }
    }

    return NULL;
}