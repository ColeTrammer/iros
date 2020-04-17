#ifndef _KERNEL_UTIL_MUTEX_H
#define _KERNEL_UTIL_MUTEX_H 1

#include <stdbool.h>

#include <kernel/proc/wait_queue.h>

typedef struct {
    struct wait_queue queue;
    int lock;
} mutex_t;

#define MUTEX_INITIALIZER \
    { WAIT_QUEUE_INITIALIZER, 0 }

void init_mutex(mutex_t *mutex);

void mutex_lock(mutex_t *mutex);
bool mutex_trylock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif /* _KERNEL_UTIL_MUTEX_H */