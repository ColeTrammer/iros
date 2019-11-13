#include <stdlib.h>

#define ATEXIT_MAX 32

static void (*volatile functions[ATEXIT_MAX])(void);
static volatile int to_call = 0;

int atexit(void (*f)(void)) {
    if (to_call >= ATEXIT_MAX) {
        return -1;
    }

    functions[to_call++] = f;
    return 0;
}

void __on_exit() {
    to_call--;
    while (to_call >= 0) {
        functions[to_call]();
        to_call--;
    }
}