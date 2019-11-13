#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>

int isatty(int fd) {
    return ioctl(fd, TISATTY) == 0;
}

int tcsetpgrp(int fd, pid_t pgid) {
    return ioctl(fd, TIOCSPGRP, &pgid);
}

int tcgetpgrp(int fd) {
    return ioctl(fd, TIOCGPGRP);
}