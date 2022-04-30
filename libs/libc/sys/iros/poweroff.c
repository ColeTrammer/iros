#include <errno.h>
#include <sys/iros.h>
#include <sys/syscall.h>

int poweroff() {
    int ret = syscall(SYS_poweroff);
    __SYSCALL_TO_ERRNO(ret);
}
