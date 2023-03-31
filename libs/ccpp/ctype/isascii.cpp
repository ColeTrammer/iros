#include <ctype.h>
#include <stdio.h>

extern "C" int isascii(int ch) {
    return int(ch >= 0 && ch <= 127);
}
