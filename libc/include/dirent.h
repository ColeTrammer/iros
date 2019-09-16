#ifndef _DIRENT_H
#define _DIRENT_H 1

#include <sys/types.h>

#define NAME_MAX 255

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int fd;
} DIR;

struct dirent {
    ino_t d_ino;
    char d_name[NAME_MAX];
};

int dirfd(DIR *dir);

int closedir(DIR *d);
DIR *fdopendir(int fd);
DIR *opendir(const char *path);

struct dirent *readdir(DIR *d);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DIRENT_H */