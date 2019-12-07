#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
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

int pause(void) {
    sigset_t set;
    sigprocmask(SIG_SETMASK, NULL, &set);
    return sigsuspend(&set);
}

int sysconf(int name) {
    switch (name) {
        case _SC_PAGE_SIZE:
            return PAGE_SIZE;
        default:
            errno = EINVAL;
            return 0;
    }
}
