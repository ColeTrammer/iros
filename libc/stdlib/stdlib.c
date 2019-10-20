#include <stdlib.h>

#ifndef __is_libk

static unsigned int seed;

void srand(unsigned int _seed) {
    seed = _seed;
}

int rand(void) {
    return rand_r(&seed);
}

// Use LCG method with same parameters as glibc
int rand_r(unsigned int *seedp) {
    *seedp = *seedp * 1103515245 + 12345;
    return (int) (*seedp % RAND_MAX);
}

#endif /* __is_libk */

int abs(int n) {
    if (n < 0) { 
        return -n; 
    }

    return n;
}

long labs(long n) {
    if (n < 0) {
        return -n;
    }

    return n;
}

div_t div(int a, int b) {
    return (div_t) { a / b, a % b };
}

ldiv_t ldiv(long a, long b) {
    return (ldiv_t) { a / b, a % b };
}