#include <string.h>

size_t strnlen(const char *s, size_t n) {
    size_t i;
    for (i = 0; i < n && s[i] != '\0'; i++)
        ;
    return i;
}
