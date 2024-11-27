#include <ctype.h>
#include <stdio.h>

extern "C" auto isprint(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int(32 <= ch && ch <= 126);
}
