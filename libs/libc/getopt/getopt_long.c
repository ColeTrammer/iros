#include <bits/do_getopt.h>
#include <getopt.h>

int getopt_long(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex) {
    return __do_getopt(argc, argv, optstring, longopts, longindex, LONG);
}
