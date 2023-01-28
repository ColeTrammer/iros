#include <sharedtest/test.h>

static __thread int x = 42;

int test(void) {
    return x++;
}
