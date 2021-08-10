#define __libc_internal

#include <errno.h>
#include <limits.h>
#include <semaphore.h>

int sem_init(sem_t *s, int pshared, unsigned int value) {
    if (value > SEM_VALUE_MAX) {
        errno = EINVAL;
        return -1;
    }

    s->__sem_flags = (pshared ? __SEM_PSHARED : 0) | __SEM_ANON;
    s->__sem_value = &s->__sem_value_storage;
    s->__sem_value_storage = value;
    return 0;
}
