#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHM_PREFIX "/dev/shm"

static char *shm_full_path(const char *name) {
    char *path = malloc(strlen(name) + strlen(SHM_PREFIX) + 1);

    strcpy(path, SHM_PREFIX);
    strcat(path, name);

    return path;
}

int shm_open(const char *name, int oflag, mode_t mode) {
    if (name[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    char *path = shm_full_path(name);
    int fd = open(name, oflag, mode);

    free(path);
    return fd;
}

int shm_unlink(const char *name) {
    if (name[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    char *path = shm_full_path(name);
    int ret = unlink(name);

    free(path);
    return ret;
}