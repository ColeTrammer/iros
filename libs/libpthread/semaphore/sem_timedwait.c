#define __libc_internal

#include <errno.h>
#include <semaphore.h>

int sem_timedwait(sem_t *__restrict s, const struct timespec *__restrict timeout) {
    if (!timeout) {
        errno = EINVAL;
        return -1;
    }

    return __sem_wait(s, timeout, 0);
}
