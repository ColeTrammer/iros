#include <string.h>

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *buffer = (unsigned char*) dest;
    const unsigned char *source = (const unsigned char*) src;
    if (source < buffer) {
        for (size_t i = n; i > 0; i--) {
            buffer[i - 1] = source[i - 1];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            buffer[i] = source[i];
        }
    }
    return buffer;
}