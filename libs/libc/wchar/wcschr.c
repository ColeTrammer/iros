#include <wchar.h>

wchar_t *wcschr(const wchar_t *wcs, wchar_t wc) {
    size_t i;
    for (i = 0; wcs[i] != '\0'; i++) {
        if (wcs[i] == wc) {
            return (wchar_t *) wcs + i;
        }
    }

    return NULL;
}
