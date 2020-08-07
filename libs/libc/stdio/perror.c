#include <errno.h>
#include <stdio.h>
#include <string.h>

void perror(const char *s) {
    const char *error_message = strerror(errno);
    if (!s || !*s) {
        fprintf(stderr, "%s\n", error_message);
    } else {
        fprintf(stderr, "%s: %s\n", s, error_message);
    }
}
