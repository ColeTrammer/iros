#include <ctype.h>
#include <stdio.h>

extern "C" int isdigit(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int('0' <= ch && ch <= '9');
}
