#include <signal.h>

int killpg(int pgid, int sig) {
    return kill(-pgid, sig);
}