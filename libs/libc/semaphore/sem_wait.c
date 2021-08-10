#define __libc_internal

#include <semaphore.h>
#include <stddef.h>

int sem_wait(sem_t *s) {
    return __sem_wait(s, NULL, 0);
}
