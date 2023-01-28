#include <wchar.h>

size_t wcslen(const wchar_t *s) {
    size_t i;
    for (i = 0; s[i] != '\0'; i++)
        ;
    return i;
}
