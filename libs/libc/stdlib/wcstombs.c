#include <stdlib.h>
#include <sys/param.h>
#include <wchar.h>

size_t wcstombs(char *dest, const wchar_t *src, size_t n) {
    size_t len = wcslen(src);
    if (!dest) {
        return len;
    }

    size_t ret = MIN(n - 1, len);
    for (size_t i = 0; i < ret; i++) {
        dest[i] = src[i];
    }
    return ret;
}
