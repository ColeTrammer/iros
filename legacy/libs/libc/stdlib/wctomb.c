#include <stdlib.h>

int wctomb(char *s, wchar_t wc) {
    if (!s) {
        return 0;
    }

    s[0] = wc;
    return 1;
}
