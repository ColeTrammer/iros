#ifndef _PTHREAD_H
#define _PTHREAD_H 1

#include <bits/pthread_attr_t.h>
#include <bits/pthread_mutex_t.h>
#include <bits/pthread_mutexattr_t.h>
#include <bits/pthread_spinlock_t.h>
#include <bits/pthread_t.h>
#include <sched.h>

#define PTHREAD_MUTEX_INITIALIZER \
    { 0 }

#ifdef __cplusplus
extern "C" {
#endif /* __plusplus */

pthread_t pthread_self(void);
int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg);
int pthread_join(pthread_t thread, void **value_ptr);
void pthread_exit(void *value_ptr) __attribute__((__noreturn__));

int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

int pthread_mutex_init(pthread_mutex_t *__restrict mutex, const pthread_mutexattr_t *__restrict mutexattr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PTHREAD_H */