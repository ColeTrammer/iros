#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int chown(const char *pathname, uid_t owner, gid_t group) {
    int ret = (int) syscall(SC_CHOWN, pathname, owner, group);
    __SYSCALL_TO_ERRNO(ret);
}
