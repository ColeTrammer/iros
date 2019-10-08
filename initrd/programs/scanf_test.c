#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

int main() {
    char *test_str = "ABC   12345:234AC1556";
    int r;
    int s;
    int t;
    int ret = sscanf(test_str, "ABC%i:%dAC%i", &r, &s, &t);
    printf("Number: %d, %d, %d, %d\n", ret, r, s, t);

    errno = 0;
    long v = strtol("2135", NULL, 0);
    if (errno == ERANGE) {
        printf("Strtol overflow\n");
    } else {
        printf("Strtol: %ld\n", v);
    }

    return 0;
}