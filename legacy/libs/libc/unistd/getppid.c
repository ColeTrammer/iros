#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>

pid_t getppid(void) {
    return (pid_t) syscall(SYS_getppid);
}
