#include <sharedtest/test.h>

static int x = 42;

int test(void) {
    return x++;
}
