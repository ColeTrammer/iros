#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int fstatat(int fd, const char *__restrict path, struct stat *__restrict stat_struct, int flag) {
    int ret = syscall(SYS_FSTATAT, fd, path, stat_struct, flag);
    __SYSCALL_TO_ERRNO(ret);
}
