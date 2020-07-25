#include <ctype.h>
#include <wctype.h>

int iswupper(wint_t c) {
    return isupper(c);
}
