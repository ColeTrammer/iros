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

double ldexp(double x, int exp) {
    if (exp > 0) {
        for (int i = exp; i > 1; i--) {
            x *= 2;
        }
        return x;
    } else if (exp < 0) {
        for (int i = exp; i < 0; i++) {
            x /= 2;
        }
        return x;
    } else {
        return 0;
    }
}

float froundf(float x) {
    if (x < 0) {
        return -froundf(-x);
    }

    // FIXME: this will only work for floats that can fit inside an int.
    if (x - (int) x >= 0.5f) {
        return ((int) x) + 1;
    }
    return (int) x;
}
