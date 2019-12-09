#ifndef __pthread_key_t_defined
#define __pthread_key_t_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    int __id;
} pthread_key_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __pthread_key_t_defined */