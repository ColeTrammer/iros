#include <stdlib.h>

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