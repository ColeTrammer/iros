#include <stdlib.h>
#include <unistd.h>

int ttyname_r(int fd, char *buf, size_t buflen) {
    return ptsname_r(fd, buf, buflen);
}
