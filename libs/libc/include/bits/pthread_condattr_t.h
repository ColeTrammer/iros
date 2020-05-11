#ifndef __pthread_condattr_t_defined
#define __pthread_condattr_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int __flags;
    clockid_t __clockid;
} pthread_condattr_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_condattr_t_defined */
