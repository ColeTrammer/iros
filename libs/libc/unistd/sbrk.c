#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

void *sbrk(intptr_t increment) {
    void *ret = (void *) syscall(SC_SBRK, increment);
    if (ret == (void *) -1) {
        errno = ENOMEM;
    }
    return ret;
}