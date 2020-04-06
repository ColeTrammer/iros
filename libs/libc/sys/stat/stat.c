#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int stat(const char *restrict path, struct stat *restrict stat_struct) {
    return fstatat(AT_FDCWD, path, stat_struct, 0);
}

int mkdir(const char *path, mode_t mode) {
    int ret = (int) syscall(SC_MKDIR, path, mode);
    __SYSCALL_TO_ERRNO(ret);
}

int chmod(const char *pathname, mode_t mode) {
    int ret = (int) syscall(SC_CHMOD, pathname, mode);
    __SYSCALL_TO_ERRNO(ret);
}

int fchmod(int fd, mode_t mode) {
    int ret = (int) syscall(SC_FCHMOD, fd, mode);
    __SYSCALL_TO_ERRNO(ret);
}

mode_t umask(mode_t mask) {
    return (mode_t) syscall(SC_UMASK, mask);
}