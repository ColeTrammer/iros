#include <time.h>
#include <stddef.h>
#include <stdio.h>

#include <sys/time.h>

time_t time(time_t *t_loc) {
    time_t t = get_time();
    if (t_loc != NULL) {
        *t_loc = t;
    }

    return t;
}

size_t strftime(char *__restrict s, size_t n, const char *__restrict format, const struct tm *__restrict tm) {
    (void) s;
    (void) n;
    (void) format;
    (void) tm;

    fputs("strftime unsupported", stderr);
    return 0;
}