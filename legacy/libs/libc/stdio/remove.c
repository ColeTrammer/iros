#include <errno.h>
#include <stdio.h>
#include <unistd.h>

int remove(const char *path) {
    int ret = unlink(path);
    if (ret == -1 && errno == EISDIR) {
        return rmdir(path);
    }

    return ret;
}
