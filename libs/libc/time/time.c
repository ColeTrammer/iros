#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>


time_t time(time_t *t_loc) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (t_loc != NULL) {
        *t_loc = tv.tv_sec;
    }

    return tv.tv_sec;
}

size_t strftime(char *__restrict s, size_t n, const char *__restrict format, const struct tm *__restrict tm) {
    (void) s;
    (void) n;
    (void) format;
    (void) tm;

    fputs("strftime unsupported", stderr);
    return 0;
}

char *ctime(const time_t *timep) {
    (void) timep;
    return "ctime unsupported";
}

static struct tm static_tm_buffer = {
    0, 0, 0, 1, 0, 119, 0, 0, 0
};

struct tm *localtime(const time_t *timep) {
    (void) timep;

    return &static_tm_buffer;
}