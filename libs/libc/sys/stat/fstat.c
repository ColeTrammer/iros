#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>

int fstat(int fd, struct stat *stat_struct) {
    return fstatat(fd, NULL, stat_struct, AT_EMPTY_PATH);
}