#include <ctype.h>
#include <stdio.h>

extern "C" auto isupper(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int('A' <= ch && ch <= 'Z');
}
