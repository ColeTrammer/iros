#include <termios.h>

int isatty(int fd) {
    struct termios t;
    int ret = tcgetattr(fd, &t);

    if (ret < 0) {
        return 0;
    }

    return 1;
}