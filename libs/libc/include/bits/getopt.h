#ifndef _BITS_GETOPT_H
#define _BITS_GETOPT_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

int getopt(int argc, char *const argv[], const char *optstring);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS_GETOPT_H */
