#include <ctype.h>
#include <stdio.h>

extern "C" int ispunct(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int((33 <= ch && ch <= 47) || (58 <= ch && ch <= 64) || (91 <= ch && ch <= 96) || (123 <= ch && ch <= 126));
}
