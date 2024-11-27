#include <ctype.h>
#include <stdio.h>

extern "C" auto isalnum(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));
}
