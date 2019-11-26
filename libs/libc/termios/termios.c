#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>

int tcgetattr(int fd, struct termios *termios_p) {
    return ioctl(fd, TCGETS, termios_p);
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
    if (optional_actions == TCSANOW) {
        return ioctl(fd, TCSETS, termios_p);
    } else if (optional_actions == TCSADRAIN) {
        return ioctl(fd, TCSETSW, termios_p);
    } else if (optional_actions == TCSAFLUSH) {
        return ioctl(fd, TCSETSF, termios_p);
    }

    errno = EINVAL;
    return -1;
}

int tcflow(int fd, int action) {
    switch (action) {
        case TCIOFF:
            return ioctl(fd, TCIOFFI);
        case TCOOFF:
            return ioctl(fd, TCOOFFI);
        case TCION:
            return ioctl(fd, TCIONI);
        case TCOON:
            return ioctl(fd, TCOONI);
        default:
            break;
    }

    errno = EINVAL;
    return -1;
}

speed_t cfgetospeed(const struct termios *termios_p) {
    (void) termios_p;
    return B38400;
}

int tcflush(int fd, int how) {
    (void) fd;
    (void) how;

    fprintf(stderr, "tcflush not supported\n");

    assert(false);
    return 0;
}