#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    char *test_str = "ABC   12345: 234AC 1556MM 0xABCDCAFEBABE1234]55555? ARBITRARYSTR -asdsaf8adqwer@ 123.4";
    int r;
    int s;
    int t;
    int x;
    char c;
    char cc;
    char str[100];
    char str2[100];
    size_t z;
    float f;
    int ret = sscanf(test_str, "A%cC%i:%dAC%iMM%zx%*c%4d5%c%s %[-a-z] %*[-a-z8@] %f", &cc, &r, &s, &t, &z, &x, &c, str, str2, &f);
    puts(str);
    printf("Number: %d, %c, %d, %d, %d, %#lX %d, %c, %s, %s\n", ret, cc, r, s, t, z, x, c, str, str2);
    printf("Float: %f\n", f);

    errno = 0;
    long v = strtol("2135", NULL, 0);
    if (errno == ERANGE) {
        printf("Strtol overflow\n");
    } else {
        printf("Strtol: %ld\n", v);
    }

    return 0;
}
