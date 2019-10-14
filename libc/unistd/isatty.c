#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>

int isatty(int fd) {
    struct termios t;
    int ret = tcgetattr(fd, &t);

    if (ret < 0) {
        return 0;
    }

    return 1;
}

int tcsetpgrp(int fd, pid_t pgid) {
    return ioctl(fd, TIOCSPGRP, &pgid);
}

int tcgetpgrp(int fd) {
    return ioctl(fd, TIOCGPGRP);
}