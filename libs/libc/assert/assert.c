#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

void __assert_failed(const char *exp, const char *file, int line, const char *func) {
    fprintf(stderr, "Assertion failed: %s in %s at %s, line %d\n", exp, func, file, line);
    abort();
}