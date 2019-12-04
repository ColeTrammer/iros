#ifndef __pthread_mutex_t_defined
#define __pthread_mutex_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int __lock;
} pthread_mutex_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_mutex_t_defined */