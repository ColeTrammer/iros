#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int dirfd(DIR *dir) {
    return dir->fd;
}

int closedir(DIR *dir) {
    int ret = close(dir->fd);
    if (ret < 0) {
        return -1;
    }

    free(dir);
    return 0;
}

DIR *fdopendir(int fd) {
    DIR *dir = calloc(1, sizeof(struct dirent));
    dir->fd = fd;
    return dir;
}

DIR *opendir(const char *path) {
    int fd = open(path, O_DIRECTORY | O_RDONLY, 0);
    if (fd == -1) {
        return NULL;
    }

    return fdopendir(fd);
}

struct dirent *readdir(DIR *d) {
    /* Should do some sys call */

    (void) d;

    return NULL;
}