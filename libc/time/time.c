#include <time.h>
#include <stddef.h>

#include <sys/time.h>

time_t time(time_t *t_loc) {
    time_t t = get_time();
    if (t_loc != NULL) {
        *t_loc = t;
    }

    return t;
}