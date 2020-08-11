#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int seteuid(uid_t uid) {
    int ret = (int) syscall(SYS_SETEUID, uid);
    __SYSCALL_TO_ERRNO(ret);
}
