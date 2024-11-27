#include <ctype.h>
#include <stdio.h>

extern "C" auto islower(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int('a' <= ch && ch <= 'z');
}
