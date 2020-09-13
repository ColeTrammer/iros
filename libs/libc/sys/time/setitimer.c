#include <sys/time.h>

int setitimer(int which, const struct itimerval *nvalp, struct itimerval *ovalp) {
    (void) which;
    (void) nvalp;
    (void) ovalp;
    return 0;
}
