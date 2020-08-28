#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t setpgid(pid_t pid, pid_t pgid) {
    pid_t ret = (pid_t) syscall(SYS_setpgid, pid, pgid);
    __SYSCALL_TO_ERRNO(ret);
}
