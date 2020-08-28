#include <errno.h>
#include <sys/resource.h>
#include <sys/syscall.h>

int setpriority(int which, id_t who, int value) {
    int ret = (int) syscall(SYS_setprioity, which, who, value);
    __SYSCALL_TO_ERRNO(ret);
}
