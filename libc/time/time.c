#include <time.h>
#include <stddef.h>

static time_t t = 1;

/* Time implementation (extremely inaccurate) */
time_t time(time_t *t_loc) {
    t++;
    if (t_loc != NULL) {
        *t_loc = t;
    }

    return t;
}