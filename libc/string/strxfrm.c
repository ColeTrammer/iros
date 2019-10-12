#include <string.h>
#include <sys/param.h>

// Should respect LC_COLLATE
size_t strxfrm(char *__restrict s1, const char *__restrict s2, size_t n) {
    size_t len = strlen(s2);
    size_t to_copy = MIN(len + 1, n);
    strncpy(s1, s2, to_copy);
    return to_copy;
}