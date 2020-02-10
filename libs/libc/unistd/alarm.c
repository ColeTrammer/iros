#include <sys/syscall.h>
#include <unistd.h>

unsigned int alarm(unsigned int seconds) {
    return (unsigned int) syscall(SC_ALARM, seconds);
}
