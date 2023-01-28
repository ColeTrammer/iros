#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getsid(pid_t pid) {
    pid_t ret = (pid_t) syscall(SYS_getsid, pid);
    __SYSCALL_TO_ERRNO(ret);
}
