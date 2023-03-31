#include <ctype.h>
#include <stdio.h>

extern "C" int isgraph(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int(33 <= ch && ch <= 126);
}
