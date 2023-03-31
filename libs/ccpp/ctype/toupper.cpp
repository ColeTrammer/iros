#include <ctype.h>
#include <stdio.h>

extern "C" int toupper(int ch) {
    if (ch == EOF) {
        return EOF;
    }
    if (islower(ch)) {
        return ch & ~0b00100000;
    }
    return ch;
}
