#ifndef _PTHREAD_H
#define _PTHREAD_H 1

#include <bits/pthread_attr_t.h>
#include <bits/pthread_mutex_t.h>
#include <bits/pthread_mutexattr_t.h>
#include <bits/pthread_spinlock_t.h>
#include <bits/pthread_t.h>
#include <sched.h>

#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM  0

#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_CREATE_JOINABLE 0

#define PTHREAD_EXPLICIT_SCHED 2
#define PTHREAD_INHERIT_SCHED  0

#define PTHREAD_MUTEX_INITIALIZER \
    { 0 }

#ifdef __cplusplus
extern "C" {
#endif /* __plusplus */

pthread_t pthread_self(void);
int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *value_ptr) __attribute__((__noreturn__));
int pthread_join(pthread_t thread, void **value_ptr);
int pthread_kill(pthread_t thread, int sig);

int pthread_getconcurrency(void);
int pthread_getschedparam(pthread_t thread, int *__restrict policy, struct sched_param *__restrict param);
int pthread_setconcurrency(int new_level);
int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);

int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *__restrict mutex, const pthread_mutexattr_t *__restrict mutexattr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_getdetachstate(const pthread_attr_t *__restrict attr, int *__restrict detachstate);
int pthread_attr_getguardsize(const pthread_attr_t *__restrict attr, size_t *__restrict guardsize);
int pthread_attr_getinheritsched(const pthread_attr_t *__restrict attr, int *__restrict inheritsched);
int pthread_attr_getschedparam(const pthread_attr_t *__restrict attr, struct sched_param *__restrict param);
int pthread_attr_getschedpolicy(const pthread_attr_t *__restrict attr, int *__restrict policy);
int pthread_attr_getscope(const pthread_attr_t *__restrict attr, int *__restrict scope);
int pthread_attr_getstack(const pthread_attr_t *__restrict attr, void **__restrict stackaddr, size_t *__restrict stacksize);
int pthread_attr_getstackaddr(const pthread_attr_t *__restrict attr, void **__restrict stackaddr);
int pthread_attr_getstacksize(const pthread_attr_t *__restrict attr, size_t *__restrict stacksize);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guardsize);
int pthread_attr_setinheritsched(pthread_attr_t *attr, int inheritsched);
int pthread_attr_setschedparam(pthread_attr_t *__restrict attr, const struct sched_param *__restrict param);
int pthread_attr_setschedpolicy(pthread_attr_t *__restrict attr, int policy);
int pthread_attr_setscope(const pthread_attr_t *attr, int scope);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

#ifdef __libc_internal

#define __PTHREAD_MAUALLY_ALLOCATED_STACK 01000

struct thread_control_block {
    struct thread_control_block *self;
    struct thread_control_block *next;
    struct thread_control_block *prev;
    pthread_t id;
    pthread_t joining_thread;
    int has_exited;
    int concurrency;
    void *exit_value;
    pthread_attr_t attributes;
};

extern struct initial_process_info __initial_process_info;
extern struct thread_control_block *__threads;

struct thread_control_block *__allocate_thread_control_block();
void __free_thread_control_block(struct thread_control_block *block);

#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PTHREAD_H */