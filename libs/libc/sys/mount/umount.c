#include <errno.h>
#include <sys/mount.h>
#include <sys/syscall.h>

int umount(const char *target) {
    int ret = (int) syscall(SYS_umount, target);
    __SYSCALL_TO_ERRNO(ret);
}
