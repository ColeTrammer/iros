#ifndef _GETOPT_H
#define _GETOPT_H 1

#include <bits/getopt.h>

#define no_argument       0
#define required_argument 1
#define optional_argument 2

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct option {
    const char *name;
    int has_arg;
    int *flag;
    int val;
};

int getopt_long(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex);
int getopt_long_only(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GETOPT_H */
