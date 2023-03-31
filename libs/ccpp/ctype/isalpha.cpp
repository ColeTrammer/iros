#include <ctype.h>
#include <stdio.h>

extern "C" int isalpha(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));
}
