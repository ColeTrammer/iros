#include <string.h>

char *stpncpy(char *__restrict dest, const char *__restrict src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    char *ret = dest + i;
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return ret;
}
