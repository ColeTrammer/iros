#ifndef _PTHREAD_H
#define _PTHREAD_H 1

#include <bits/pthread_attr_t.h>
#include <bits/pthread_spinlock_t.h>
#include <bits/pthread_t.h>
#include <sched.h>

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PTHREAD_H */