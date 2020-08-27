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

double tanh(double x) {
    return x;
}

double tan(double x) {
    return x;
}

double sqrt(double x) {
    return x;
}

double sinh(double x) {
    return x;
}

double log10(double x) {
    return x;
}

double log(double x) {
    return x;
}

double exp(double x) {
    return x;
}

double cosh(double x) {
    return x;
}

double atan2(double y, double x) {
    // FIXME: this is wrong.
    return atan(y / x);
}

double atan(double x) {
    return x;
}

double asin(double x) {
    return x;
}

double acos(double x) {
    return x;
}

double fmod(double x, double y) {
    return (int) x % (int) y;
}

double frexp(double x, int *exp) {
    (void) exp;
    return x;
}

double modf(double x, double *iptr) {
    (void) iptr;
    return x;
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

double ceil(double x) {
    return x;
}

float roundf(float x) {
    if (x < 0) {
        return -roundf(-x);
    }

    // FIXME: this will only work for floats that can fit inside an int.
    if (x - (int) x >= 0.5f) {
        return ((int) x) + 1;
    }
    return (int) x;
}
