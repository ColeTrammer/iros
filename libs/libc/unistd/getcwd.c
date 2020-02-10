#include <errno.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

char *getcwd(char *buf, size_t size) {
    if (buf == NULL) {
        if (size == 0) {
            size = 4096;
        }
        buf = malloc(size);
    }

    char *ret = (char *) syscall(SC_GETCWD, buf, size);
    if (ret == NULL) {
        errno = ERANGE;
    }

    return ret;
}