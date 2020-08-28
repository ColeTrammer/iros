#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int rename(const char *old, const char *new_path) {
    int ret = (int) syscall(SYS_rename, old, new_path);
    __SYSCALL_TO_ERRNO(ret);
}
