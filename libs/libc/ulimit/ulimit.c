#include <stdio.h>
#include <ulimit.h>

long ulimit(int cmd, ...) {
    fprintf(stderr, "ulimit not implemented: %d\n", cmd);
    return 0;
}
