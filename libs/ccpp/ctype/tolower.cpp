#include <ctype.h>
#include <stdio.h>

extern "C" int tolower(int ch) {
    if (ch == EOF) {
        return EOF;
    }
    if (isupper(ch)) {
        return ch | 0b00100000;
    }
    return ch;
}
