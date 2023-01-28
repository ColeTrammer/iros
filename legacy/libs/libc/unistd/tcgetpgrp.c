#include <sys/ioctl.h>
#include <unistd.h>

int tcgetpgrp(int fd) {
    return ioctl(fd, TIOCGPGRP);
}
