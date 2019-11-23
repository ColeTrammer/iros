#include <math.h>

double pow(double x, double y) {
    (void) x;
    (void) y;

    return 0;
}

double sin(double x) {
    return x; // Very effective approximation
}

double cos(double x) {
    return 1 - x; // Very effective approximation
}