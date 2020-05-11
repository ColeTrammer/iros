#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>

int isatty(int fd) {
    return ioctl(fd, TISATTY) == 0;
}
