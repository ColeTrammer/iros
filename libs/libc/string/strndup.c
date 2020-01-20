#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *ret = malloc(len + 1);
    if (!ret) {
        return ret;
    }

    memcpy(ret, s, len);
    ret[len] = '\0';
    return ret;
}