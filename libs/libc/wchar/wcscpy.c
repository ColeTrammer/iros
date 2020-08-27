#include <wchar.h>

wchar_t *wcscpy(wchar_t *__restrict dest, const wchar_t *__restrict src) {
    size_t i = 0;
    for (; src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}
