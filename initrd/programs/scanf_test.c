#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

int main() {
    char *test_str = "ABC   12345: 234AC 1556MM 0xABCDCAFEBABE1234    \t\v 55555?";
    int r;
    int s;
    int t;
    int x;
    char c;
    char cc;
    size_t z;
    int ret = sscanf(test_str, "A%cC%i:%dAC%iMM%zx %4d5%c", &cc, &r, &s, &t, &z, &x, &c);
    printf("Number: %d, %c, %d, %d, %d, %#lX %d, %c\n", ret, cc, r, s, t, z, x, c);

    errno = 0;
    long v = strtol("2135", NULL, 0);
    if (errno == ERANGE) {
        printf("Strtol overflow\n");
    } else {
        printf("Strtol: %ld\n", v);
    }

    return 0;
}