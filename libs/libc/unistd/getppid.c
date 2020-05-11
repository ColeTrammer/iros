#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getppid(void) {
    pid_t ret = (pid_t) syscall(SC_GETPID);
    __SYSCALL_TO_ERRNO(ret);
}
