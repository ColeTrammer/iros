#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

int lstat(const char *__restrict path, struct stat *__restrict stat_struct) {
    int ret = (int) syscall(SC_LSTAT, path, stat_struct);
    __SYSCALL_TO_ERRNO(ret);
}