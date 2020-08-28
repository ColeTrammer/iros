#include <errno.h>
#include <limits.h>
#include <sys/syscall.h>
#include <unistd.h>

int nice(int inc) {
    int ret = (int) syscall(SYS_nice, inc);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret - NZERO;
}
