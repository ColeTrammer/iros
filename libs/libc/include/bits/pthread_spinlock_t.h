#ifndef __pthread_spinlock_t_defined
#define __pthread_spinlock_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int lock;
} pthread_spinlock_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_spinlock_t_defined */