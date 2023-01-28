#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

int lstat(const char *__restrict path, struct stat *__restrict stat_struct) {
    return fstatat(AT_FDCWD, path, stat_struct, AT_SYMLINK_NOFOLLOW);
}
