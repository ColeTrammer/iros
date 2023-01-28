#include <stdlib.h>

int mbtowc(wchar_t *pwc, const char *s, size_t n) {
    if (!s) {
        return 0;
    }

    if (n == 0) {
        return -1;
    }

    wchar_t result = *s;
    if (!result) {
        return 0;
    }

    if (pwc) {
        *pwc = result;
    }
    return 1;
}
