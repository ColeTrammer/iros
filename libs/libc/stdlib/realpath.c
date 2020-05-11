#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/syscall.h>

char *realpath(const char *__restrict path, char *__restrict resolved_path) {
    if (!resolved_path) {
        resolved_path = malloc(PATH_MAX);
        if (!resolved_path) {
            return NULL;
        }
    }

    int ret = (int) syscall(SC_REALPATH, path, resolved_path, PATH_MAX);
    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    return resolved_path;
}
