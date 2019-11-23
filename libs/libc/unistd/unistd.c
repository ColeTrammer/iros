#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

pid_t getpgrp(void) {
    return getpgid(0);
}

int creat(const char *pathname, mode_t mode) {
    return open(pathname, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int lstat(const char *__restrict path, struct stat *__restrict stat_struct) {
    return stat(path, stat_struct);
}