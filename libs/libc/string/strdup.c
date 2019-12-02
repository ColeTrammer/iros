#include <stdlib.h>
#include <string.h>

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;
    void *b = malloc(len);

    memcpy(b, s, len);
    return b;
}