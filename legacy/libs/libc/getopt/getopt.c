#include <bits/do_getopt.h>
#include <getopt.h>
#include <stddef.h>

int getopt(int argc, char *const argv[], const char *optstring) {
    return __do_getopt(argc, argv, optstring, NULL, NULL, SHORT);
}
