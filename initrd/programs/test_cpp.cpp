#include <stdio.h>
#include <stdlib.h>

int main() {
    double x = 1.123;
    printf("%g\n", x);

    printf("%.2f\n", atof("1.52"));

    printf("%.6f\n", atof("-100.015446"));

    puts("Hello world\n");

    return 0;
}