#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int setgid(gid_t gid) {
    int ret = (int) syscall(SC_SETGID, gid);
    __SYSCALL_TO_ERRNO(ret);
}
