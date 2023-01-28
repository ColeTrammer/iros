#ifndef _BITS_DO_GETOPT_H
#define _BITS_DO_GETOPT_H 1

struct option;

enum getopt_mode {
    SHORT,
    LONG,
    LONG_ONLY,
};

int __do_getopt(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex, enum getopt_mode mode);

#endif /* _BITS_DO_GETOPT_H */
