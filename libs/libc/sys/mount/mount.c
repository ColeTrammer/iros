#include <errno.h>
#include <sys/mount.h>
#include <sys/syscall.h>

int mount(const char *source, const char *target, const char *fs_type, unsigned long flags, const void *data) {
    int ret = (int) syscall(SYS_mount, source, target, fs_type, flags, data);
    __SYSCALL_TO_ERRNO(ret);
}
