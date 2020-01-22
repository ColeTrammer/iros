#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t fork() {
    pid_t ret = (pid_t) syscall(SC_FORK);
    __SYSCALL_TO_ERRNO(ret);
}