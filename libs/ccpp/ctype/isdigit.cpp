#include <ctype.h>
#include <stdio.h>

extern "C" auto isdigit(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int('0' <= ch && ch <= '9');
}
