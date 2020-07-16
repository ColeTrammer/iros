#ifndef _DIRENT_H
#define _DIRENT_H 1

#include <bits/ino_t.h>

#define NAME_MAX 255

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct dirent {
    ino_t d_ino;
    char d_name[NAME_MAX];
};

typedef struct {
    int fd;
    struct dirent entry;
} DIR;

int dirfd(DIR *dir);

int closedir(DIR *d);
DIR *fdopendir(int fd);
DIR *opendir(const char *path);

struct dirent *readdir(DIR *d);
int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);

int alphasort(const struct dirent **a, const struct dirent **b);
int scandir(const char *dirp, struct dirent ***list, int (*filter)(const struct dirent *d),
            int (*conpar)(const struct dirent **a, const struct dirent **b));

void rewinddir(DIR *d);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DIRENT_H */
