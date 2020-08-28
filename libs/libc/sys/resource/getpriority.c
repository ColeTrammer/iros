#include <errno.h>
#include <limits.h>
#include <sys/resource.h>
#include <sys/syscall.h>

int getpriority(int which, id_t id) {
    int ret = (int) syscall(SYS_getpriority, which, id);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret - NZERO;
}
