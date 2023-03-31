#include <ctype.h>
#include <stdio.h>

extern "C" int isupper(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int('A' <= ch && ch <= 'Z');
}
