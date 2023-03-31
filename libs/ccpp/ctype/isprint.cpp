#include <ctype.h>
#include <stdio.h>

extern "C" int isprint(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int(32 <= ch && ch <= 126);
}
