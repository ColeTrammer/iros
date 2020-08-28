#include <stdlib.h>

size_t mbstowcs(wchar_t *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; src[i] != '\0' && i < n; i++) {
        dest[i] = src[i];
    }

    return i;
}
