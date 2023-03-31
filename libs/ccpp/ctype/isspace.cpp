#include <ctype.h>
#include <stdio.h>

extern "C" int isspace(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int((9 <= ch && ch <= 13) || (ch == ' '));
}
