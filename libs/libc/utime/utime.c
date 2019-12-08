#include <stdio.h>
#include <utime.h>

int utime(const char *filename, const struct utimbuf *times) {
    (void) filename;
    (void) times;

    fprintf(stderr, "utime not supported\n");
    return 0;
}