#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

int execve(const char *file, char *const argv[], char *const envp[]) {
    int ret = (int) syscall(SC_EXECVE, file, argv, envp);
    __SYSCALL_TO_ERRNO(ret);
}
