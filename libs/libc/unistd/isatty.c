#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>

int isatty(int fd) {
    return ioctl(fd, TISATTY) == 0;
}

int tcsetpgrp(int fd, pid_t pgid) {
    return ioctl(fd, TIOCSPGRP, &pgid);
}

int tcgetpgrp(int fd) {
    return ioctl(fd, TIOCGPGRP);
}

char static_name_buffer[20];

char *ttyname(int fd) {
    int ret = ptsname_r(fd, static_name_buffer, 20);
    if (ret == -1) {
        return NULL;
    }

    return static_name_buffer;
}

int ttyname_r(int fd, char *buf, size_t buflen) {
    return ptsname_r(fd, buf, buflen);
}