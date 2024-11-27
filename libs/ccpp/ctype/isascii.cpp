#include <ctype.h>
#include <stdio.h>

extern "C" auto isascii(int ch) -> int {
    return int(ch >= 0 && ch <= 127);
}
