#include <sys/os_2.h>
#include <sys/syscall.h>

int tgkill(int tgid, int tid, int signum) {
    return syscall(SC_TGKILL, tgid, tid, signum);
}
