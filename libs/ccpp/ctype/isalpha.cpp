#include <ctype.h>
#include <stdio.h>

extern "C" auto isalpha(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));
}
