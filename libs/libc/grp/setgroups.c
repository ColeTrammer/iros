#include <errno.h>
#include <grp.h>
#include <sys/syscall.h>

int setgroups(size_t size, const gid_t *list) {
    int ret = syscall(SYS_SETGROUPS, size, list);
    __SYSCALL_TO_ERRNO(ret);
}
