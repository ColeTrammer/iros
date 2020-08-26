#ifndef _PTHREAD_H
#define _PTHREAD_H 1

// sched.h comes first for clockid_t (needed by pthread_condattr_t)
#include <sched.h>
#include <time.h>

#include <bits/pthread_attr_t.h>
#include <bits/pthread_barrier_t.h>
#include <bits/pthread_barrierattr_t.h>
#include <bits/pthread_cond_t.h>
#include <bits/pthread_key_t.h>
#include <bits/pthread_mutex_t.h>
#include <bits/pthread_once_t.h>
#include <bits/pthread_rwlock_t.h>
#include <bits/pthread_rwlockattr_t.h>
#include <bits/pthread_spinlock_t.h>
#include <bits/pthread_t.h>

#define PTHREAD_BARRIER_SERIAL_THREAD 1

#define PTHREAD_CANCEL_ASYNCHRONOUS 4
#define PTHREAD_CANCEL_DEFERRED     0

#define PTHREAD_CANCEL_ENABLE  0
#define PTHREAD_CANCEL_DISABLE 8

#define PTHREAD_CANCELED ((void *) -1)

#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM  0

#define PTHREAD_CREATE_DETACHED 1
#define PTHREAD_CREATE_JOINABLE 0

#define PTHREAD_EXPLICIT_SCHED 2
#define PTHREAD_INHERIT_SCHED  0

#define PTHREAD_PROCESS_SHARED  0
#define PTHREAD_PROCESS_PRIVATE 1

#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_RECURSIVE  4
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_STALLED    0
#define PTHREAD_MUTEX_ROBUST     8

#define PTHREAD_ONCE_INIT 0

#define PTHREAD_COND_INITIALIZER \
    {                            \
        0, { 0, 0 }              \
    }

#define PTHREAD_MUTEX_INITIALIZER                                                                                                   \
    {                                                                                                                               \
        0, 0, { 0 }, { 0, 0, (unsigned int *) 0, (struct __locked_robust_mutex_node *) 0, (struct __locked_robust_mutex_node *) 0 } \
    }

#define PTHREAD_RWLOCK_INITIALIZER \
    { 0 }

#ifdef __cplusplus
extern "C" {
#endif /* __plusplus */

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

int pthread_barrierattr_destroy(pthread_barrierattr_t *barrierattr);
int pthread_barrierattr_init(pthread_barrierattr_t *barrierattr);
int pthread_barrierattr_getpshared(const pthread_barrierattr_t *__restrict barrierattr, int *__restrict pshared);
int pthread_barrierattr_setpshared(pthread_barrierattr_t *barrierattr, int pshared);

int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_barrier_init(pthread_barrier_t *__restrict barrier, const pthread_barrierattr_t *__restrict attr, unsigned int count);
int pthread_barrier_wait(pthread_barrier_t(barrier));

pthread_t pthread_self(void);
int pthread_create(pthread_t *__restrict thread, const pthread_attr_t *__restrict attr, void *(*start_routine)(void *arg),
                   void *__restrict arg);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *value_ptr) __attribute__((__noreturn__));
int pthread_join(pthread_t thread, void **value_ptr);

int pthread_getcpuclockid(pthread_t thread, clockid_t *clock_id);
int pthread_getconcurrency(void);
int pthread_getschedparam(pthread_t thread, int *__restrict policy, struct sched_param *__restrict param);
int pthread_setconcurrency(int new_level);
int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param);
int pthread_setschedprio(pthread_t thread, int prio);

int pthread_once(pthread_once_t *once_control, void (*init_function)(void));

int pthread_condattr_init(pthread_condattr_t *condattr);
int pthread_condattr_destroy(pthread_condattr_t *condattr);
int pthread_condattr_getclock(pthread_condattr_t *__restrict condattr, clockid_t *__restrict clock);
int pthread_condattr_getpshared(pthread_condattr_t *__restrict condattr, int *__restrict pshared);
int pthread_condattr_setclock(pthread_condattr_t *condattr, clockid_t clock);
int pthread_condattr_setpshared(pthread_condattr_t *__restrict condattr, int pshared);

int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_init(pthread_cond_t *__restrict cond, const pthread_condattr_t *__restrict attr);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_timedwait(pthread_cond_t *__restrict cond, pthread_mutex_t *__restrict mutex, const struct timespec *__restrict timeout);
int pthread_cond_wait(pthread_cond_t *__restrict cond, pthread_mutex_t *__restrict mutex);

int pthread_spin_destroy(pthread_spinlock_t *lock);
int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
int pthread_spin_lock(pthread_spinlock_t *lock);
int pthread_spin_trylock(pthread_spinlock_t *lock);
int pthread_spin_unlock(pthread_spinlock_t *lock);

int pthread_mutexattr_init(pthread_mutexattr_t *mutexattr);
int pthread_mutexattr_destroy(pthread_mutexattr_t *mutexattr);
int pthread_mutexattr_getpshared(pthread_mutexattr_t *__restrict mutexattr, int *__restrict pshared);
int pthread_mutexattr_getrobust(const pthread_mutexattr_t *__restrict mutexattr, int *__restrict robust);
int pthread_mutexattr_gettype(const pthread_mutexattr_t *__restrict mutexattr, int *__restrict type);
int pthread_mutexattr_setpshared(pthread_mutexattr_t *mutexattr, int pshared);
int pthread_mutexattr_setrobust(pthread_mutexattr_t *mutexattr, int robust);
int pthread_mutexattr_settype(pthread_mutexattr_t *mutexattr, int type);

int pthread_mutex_consistent(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_init(pthread_mutex_t *__restrict mutex, const pthread_mutexattr_t *__restrict mutexattr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *__restrict mutex, const struct timespec *__restrict timeout);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *k));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *rwlockattr);
int pthread_rwlockattr_getpshared(pthread_rwlockattr_t *__restrict rwlockattr, int *__restrict pshared);
int pthread_rwlockattr_init(pthread_rwlockattr_t *rwlockattr);
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *rwlockattr, int pshared);

int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
int pthread_rwlock_init(pthread_rwlock_t *__restrict rwlock, const pthread_rwlockattr_t *__restrict rwlockattr);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_timedrdlock(pthread_rwlock_t *__restrict rwlock, const struct timespec *__restrict timeout);
int pthread_rwlock_timedwrlock(pthread_rwlock_t *__restrict rwlock, const struct timespec *__restrict timeout);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);

int pthread_cancel(pthread_t thread);
int pthread_setcancelstate(int state, int *oldstate);
int pthread_setcanceltype(int type, int *oldtype);
void pthread_testcancel(void);

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
int pthread_attr_setscope(pthread_attr_t *attr, int scope);
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);

struct __pthread_cleanup_handler {
    struct __pthread_cleanup_handler *__next;
    void (*__func)(void *arg);
    void *__arg;
};

extern __thread struct __pthread_cleanup_handler *__cleanup_handlers;

#define pthread_cleanup_push(rtn, arg)                      \
    {                                                       \
        struct __pthread_cleanup_handler __cleanup_handler; \
        __cleanup_handler.__func = rtn;                     \
        __cleanup_handler.__arg = arg;                      \
        __cleanup_handler.__next = __cleanup_handlers;      \
        __cleanup_handlers = &__cleanup_handler;

// clang-format off
#define pthread_cleanup_pop(ex)                                   \
        __cleanup_handlers = __cleanup_handler.__next;            \
        if (ex) {                                                 \
            (*__cleanup_handler.__func)(__cleanup_handler.__arg); \
        }                                                         \
    }
// clang-format on

#ifdef __libc_internal

#define __PTHREAD_ONCE_IN_PROGRESS 1
#define __PTHREAD_ONCE_FINISHED    2

#define __PTHREAD_MUTEX_INCONSISTENT  16
#define __PTHREAD_MUTEX_UNRECOVERABLE 32

#define __PTHREAD_TIMER_SIGNAL  30
#define __PTHREAD_CANCEL_SIGNAL 31

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
    union {
        size_t number;
        void *pointer;
    } * dynamic_thread_vector;
    size_t dynamic_thread_vector_max;
    void **pthread_specific_data;
    struct __locked_robust_mutex_node *locked_robust_mutex_node_list_head;
    pthread_attr_t attributes;
};

extern __attribute__((weak)) struct initial_process_info *__initial_process_info;
extern struct thread_control_block *__threads;
extern int __cancelation_setup;

struct thread_control_block *__allocate_thread_control_block(void);
void __free_thread_control_block(struct thread_control_block *block);

struct thread_control_block *__get_self(void);

void pthread_specific_run_destructors(struct thread_control_block *thread);

void setup_cancelation_handler(void);

#endif /* __libc_internal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _PTHREAD_H */
