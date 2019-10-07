#include <unistd.h>

char *optarg = NULL;
int optind = 0, opterr = 1, optopt = 1;

int getopt(int argc, char *const argv[], const char *optstring) {
    (void) argc;
    (void) argv;
    (void) optstring;

    return -1;
}