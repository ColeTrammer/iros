#define __libc_internal

#include <errno.h>
#include <semaphore.h>

int sem_destroy(sem_t *sem) {
    if (!(sem->__sem_flags & __SEM_ANON)) {
        errno = EINVAL;
        return -1;
    }

    sem->__sem_flags = -1;
    return 0;
}
