#include <sys/syscall.h>
#include <unistd.h>

pid_t getpid() {
    return (pid_t) syscall(SC_GETPID);
}