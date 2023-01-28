#include <errno.h>
#include <sys/resource.h>
#include <sys/syscall.h>

int getrusage(int who, struct rusage *rusage) {
    int ret = syscall(SYS_getrusage, who, rusage);
    __SYSCALL_TO_ERRNO(ret);
}
