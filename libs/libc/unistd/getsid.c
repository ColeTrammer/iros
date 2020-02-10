#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getsid(pid_t pid) {
    pid_t ret = (pid_t) syscall(SC_GETSID, pid);
    __SYSCALL_TO_ERRNO(ret);
}