#include <signal.h>
#include <sys/os_2.h>

int raise(int signum) {
    // Make sure to signal the current thread, not just a random one in the process
    return tgkill(0, 0, signum);
}
