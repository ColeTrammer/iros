#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int setegid(gid_t gid) {
    int ret = (int) syscall(SYS_SETEGID, gid);
    __SYSCALL_TO_ERRNO(ret);
}
