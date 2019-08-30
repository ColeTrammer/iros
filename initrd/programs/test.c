#include <string.h>
#include <stdbool.h>
#include <stdio.h>

static char s[12];

int main() {
    memset(s, 'A', 11);
    puts(s);

    return 1;
}