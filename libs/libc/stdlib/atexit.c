#include <limits.h>
#include <stdlib.h>

extern void (*functions[ATEXIT_MAX])(void);
extern int to_call;

int atexit(void (*f)(void)) {
    if (to_call >= ATEXIT_MAX) {
        return -1;
    }

    functions[to_call++] = f;
    return 0;
}
