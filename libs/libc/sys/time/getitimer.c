#include <sys/time.h>

int getitimer(int which, struct itimerval *value) {
    (void) which;
    (void) value;
    return 0;
}
