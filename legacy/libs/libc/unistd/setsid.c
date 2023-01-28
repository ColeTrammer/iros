#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t setsid(void) {
    pid_t ret = (pid_t) syscall(SYS_setsid);
    __SYSCALL_TO_ERRNO(ret);
}
