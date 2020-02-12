#include <signal.h>

int sigfillset(sigset_t *set) {
    *set = ~0ULL;
    return 0;
}