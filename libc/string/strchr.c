#include <string.h>

char *strchr(const char *s, int c) {
    char val = (char) c;

    size_t i = 0;
    while (s[i] != '\0') {
        if (s[i++] == val) {
            return (char*) s + i;
        }
    }

    return NULL;
}