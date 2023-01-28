#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int getgroups(int size, gid_t list[]) {
    int ret = syscall(SYS_getgroups, size, list);
    __SYSCALL_TO_ERRNO(ret);
}
