#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H 1

#include <fcntl.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int sem_t;

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
