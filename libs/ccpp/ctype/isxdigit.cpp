#include <ctype.h>
#include <stdio.h>

extern "C" auto isxdigit(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'));
}
