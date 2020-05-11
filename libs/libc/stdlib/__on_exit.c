#include <limits.h>

void (*functions[ATEXIT_MAX])(void);
int to_call = 0;

void __on_exit() {
    to_call--;
    while (to_call >= 0) {
        functions[to_call]();
        to_call--;
    }
}
