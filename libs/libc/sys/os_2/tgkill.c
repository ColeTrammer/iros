#include <sys/os_2.h>
#include <sys/syscall.h>

int tgkill(int tgid, int tid, int signum) {
    return syscall(SYS_tgkill, tgid, tid, signum);
}
