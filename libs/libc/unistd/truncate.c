#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int truncate(const char *path, off_t length) {
    int ret = (int) syscall(SC_TRUNCATE, path, length);
    return ret;
}
