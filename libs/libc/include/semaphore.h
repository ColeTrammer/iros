#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H 1

#include <bits/lock.h>
#include <fcntl.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __sem {
    struct __sem *__sem_open_next;
    struct __sem *__sem_open_prev;
    int *__sem_value;
    int __sem_ref_count;
#define __SEM_ANON    1
#define __SEM_PSHARED 2
    int __sem_flags;
    union {
        int __sem_value_storage;
        char __sem_open_name[0];
    };
};

#ifdef __libc_internal
__attribute__((visibility("internal"))) struct __lock __sem_open_lock;
__attribute__((visibility("internal"))) struct __sem *__sem_open_head;
__attribute__((visibility("internal"))) struct __sem *__sem_open_tail;
__attribute__((visibility("internal"))) int __sem_wait(struct __sem *__restrict s, const struct timespec *__restrict timeout, int try_only);
#endif /* __libc_internal */

typedef struct __sem sem_t;

int sem_close(sem_t *s);
int sem_destroy(sem_t *s);
int sem_getvalue(sem_t *__restrict s, int *__restrict value);
int sem_init(sem_t *s, int pshared, unsigned int value);
sem_t *sem_open(const char *name, int oflag, ...);
int sem_post(sem_t *s);
int sem_timedwait(sem_t *__restrict s, const struct timespec *__restrict timeout);
int sem_trywait(sem_t *s);
int sem_unlink(const char *name);
int sem_wait(sem_t *s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define SEM_FAILED ((sem_t *) -1)

#endif /* _SEMAPHORE_H */
