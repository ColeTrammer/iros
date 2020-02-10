#include <sys/syscall.h>
#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
    return (unsigned int) syscall(SC_SLEEP, seconds);
}