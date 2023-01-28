#define __libc_internal

#include <semaphore.h>
#include <stdatomic.h>
#include <sys/iros.h>

int sem_getvalue(sem_t *__restrict s, int *__restrict value_p) {
    int value = atomic_load(s->__sem_value);
    *value_p = value & ~MUTEX_WAITERS;
    return 0;
}
