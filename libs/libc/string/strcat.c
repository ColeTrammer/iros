#include <string.h>

char *strcat(char *__restrict dest, const char *__restrict src) {
    size_t end = strlen(dest);
    size_t i;
    for (i = 0; src[i] != '\0'; i++) {
        dest[end + i] = src[i];
    }

    dest[end + i] = '\0';
    return dest;
}
