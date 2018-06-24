#include <string.h>

void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    unsigned char *buffer = (unsigned char*) dest;
    const unsigned char *source = (const unsigned char*) src;
    for (size_t i = 0; i < n; i++) {
        buffer[i] = source[i];
    }
    return buffer;
}