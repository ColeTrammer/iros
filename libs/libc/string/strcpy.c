#include <string.h>

char *strcpy(char *__restrict dest, const char *__restrict src) {
    size_t i = 0;
    for (; src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}