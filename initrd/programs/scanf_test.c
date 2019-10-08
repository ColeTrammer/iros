#include <stdio.h>

int main() {
    char *test_str = "ABC12345:234AC1556";
    int r;
    int s;
    int t;
    int ret = sscanf(test_str, "ABC%i:%iAC%i", &r, &s, &t);
    printf("Number: %d, %d, %d, %d\n", ret, r, s, t);
    (void) r;
    return 0;
}