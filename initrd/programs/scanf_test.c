#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

int main() {
    char *test_str = "ABC   12345: 234AC 1556MM 0xABCDCAFEBABE1234";
    int r;
    int s;
    int t;
    size_t z;
    int ret = sscanf(test_str, "ABC%i:%dAC%iMM%zx", &r, &s, &t, &z);
    printf("Number: %d, %d, %d, %d, %#lX\n", ret, r, s, t, z);

    errno = 0;
    long v = strtol("2135", NULL, 0);
    if (errno == ERANGE) {
        printf("Strtol overflow\n");
    } else {
        printf("Strtol: %ld\n", v);
    }

    return 0;
}