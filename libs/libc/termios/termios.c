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