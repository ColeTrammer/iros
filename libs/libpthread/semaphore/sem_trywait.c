#define __libc_internal

#include <semaphore.h>

int sem_trywait(sem_t *s) {
    return __sem_wait(s, NULL, 1);
}
