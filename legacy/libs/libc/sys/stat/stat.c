#include <fcntl.h>
#include <sys/stat.h>

int stat(const char *restrict path, struct stat *restrict stat_struct) {
    return fstatat(AT_FDCWD, path, stat_struct, 0);
}
