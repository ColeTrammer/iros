#ifndef _KERNEL_UTIL_MUTEX_H
#define _KERNEL_UTIL_MUTEX_H 1

#include <stdbool.h>

#include <kernel/proc/wait_queue.h>

typedef struct {
    struct wait_queue queue;
    int lock;
} mutex_t;

#define MUTEX_INITIALIZER(m) \
    { WAIT_QUEUE_INITIALIZER((m).queue), 0 }

void init_mutex_internal(mutex_t *mutex, const char *func);
void mutex_lock_internal(mutex_t *mutex, const char *func);
bool mutex_trylock_internal(mutex_t *mutex, const char *func);
void mutex_unlock_internal(mutex_t *mutex, const char *func);

#define init_mutex(m)    init_mutex_internal(m, __func__)
#define mutex_lock(m)    mutex_lock_internal(m, __func__)
#define mutex_trylock(m) mutex_trylock_internal(m, __func__)
#define mutex_unlock(m)  mutex_unlock_internal(m, __func__)

#endif /* _KERNEL_UTIL_MUTEX_H */
