#include <stdlib.h>
#include <unistd.h>

char static_name_buffer[20];

char *ttyname(int fd) {
    int ret = ptsname_r(fd, static_name_buffer, 20);
    if (ret == -1) {
        return NULL;
    }

    return static_name_buffer;
}