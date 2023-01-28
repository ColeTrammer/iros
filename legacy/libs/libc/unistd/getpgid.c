#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getpgid(pid_t pid) {
    pid_t ret = (pid_t) syscall(SYS_getpgid, pid);
    __SYSCALL_TO_ERRNO(ret);
}
