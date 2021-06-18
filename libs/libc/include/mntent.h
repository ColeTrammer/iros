#ifndef _MNTENT_H
#define _MNTENT_H 1

#include <bits/file.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mntent {
    char *mnt_fsname;
    char *mnt_dir;
    char *mnt_type;
    char *mnt_opts;
};

FILE *setmntent(const char *path, const char *type);
struct mntent *getmntent(FILE *stream);
int endmntent(FILE *stream);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MNTENT_H */
