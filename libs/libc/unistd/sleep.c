#include <errno.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
    struct timespec sec = { .tv_sec = seconds, .tv_nsec = 0 };
    struct timespec rem;
    int ret = nanosleep(&sec, &rem);
    if (ret == -EINTR) {
        return rem.tv_sec + rem.tv_nsec ? 1 : 0;
    }

    return 0;
}
