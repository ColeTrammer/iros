#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int setuid(uid_t uid) {
    int ret = (int) syscall(SC_SETUID, uid);
    __SYSCALL_TO_ERRNO(ret);
}
