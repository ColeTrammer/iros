#include <string.h>

char *strchr(const char *s, int c) {
    char val = (char) c;

    size_t i;
    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == val) {
            return (char*) s + i;
        }
    }

    return NULL;
}