#include <ctype.h>
#include <stdio.h>

extern "C" int iscntrl(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int((ch >= 0 && ch <= 31) || (ch == 127));
}
