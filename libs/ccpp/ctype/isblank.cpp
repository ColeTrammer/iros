#include <ctype.h>
#include <stdio.h>

extern "C" int isblank(int ch) {
    if (ch == EOF) {
        return 0;
    }
    return int((ch == '\t') || (ch == ' '));
}
