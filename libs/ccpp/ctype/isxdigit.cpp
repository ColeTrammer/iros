#include <ctype.h>
#include <stdio.h>

extern "C" int isxdigit(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'));
}
