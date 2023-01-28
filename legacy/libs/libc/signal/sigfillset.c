#include <signal.h>
#include <stdint.h>

int sigfillset(sigset_t *set) {
    *set = ~UINT64_C(0);
    return 0;
}
