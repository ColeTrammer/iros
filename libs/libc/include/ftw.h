#ifndef _FTW_H
#define _FTW_H 1

#include <sys/stat.h>

#define FTW_CHDIR 1
#define FTW_DEPTH 2
#define FTW_MOUNT 4
#define FTW_PHYS  8

#define FTW_D   1
#define FTW_DNR 2
#define FTW_DP  3
#define FTW_F   4
#define FTW_NS  5
#define FTW_SL  6
#define FTW_SLN 7

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct FTW {
    int base;
    int level;
};

int ftw(const char *path, int (*fn)(const char *path, const struct stat *stat_struct, int type), int fd_limit);
int nftw(const char *path, int (*fn)(const char *path, const struct stat *stat_struct, int type, struct FTW *info), int fd_limit,
         int flags);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FTW_H */
