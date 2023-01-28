#include <errno.h>
#include <sys/syscall.h>
#include <sys/times.h>

clock_t times(struct tms *buf) {
    clock_t ret = (clock_t) syscall(SYS_times, buf);

    // times is special since the clock_t can overflow to a negative number
    if (ret == (clock_t) -EFAULT) {
        errno = EFAULT;
        return -1;
    }
    return ret;
}
