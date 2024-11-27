#include <ctype.h>
#include <stdio.h>

extern "C" auto iscntrl(int ch) -> int {
    if (ch == EOF) {
        return 0;
    }
    return int((ch >= 0 && ch <= 31) || (ch == 127));
}
