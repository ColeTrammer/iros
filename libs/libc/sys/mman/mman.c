#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SHM_PREFIX "/dev/shm"

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    void *ret = (void *) syscall(SC_MMAP, addr, length, prot, flags, fd, offset);
    if ((long) ret < 0 && (long) ret > -EMAXERRNO) {
        errno = -((long) ret);
        return MAP_FAILED;
    }

    return ret;
}

int munmap(void *addr, size_t length) {
    int ret = (int) syscall(SC_MUNMAP, addr, length);
    __SYSCALL_TO_ERRNO(ret);
}

int mprotect(void *addr, size_t length, int prot) {
    int ret = (int) syscall(SC_MPROTECT, addr, length, prot);
    __SYSCALL_TO_ERRNO(ret);
}

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
    int fd = open(path, oflag, mode);

    free(path);
    return fd;
}

int shm_unlink(const char *name) {
    if (name[0] != '/') {
        errno = EINVAL;
        return -1;
    }

    char *path = shm_full_path(name);
    int ret = unlink(path);

    free(path);
    return ret;
}
