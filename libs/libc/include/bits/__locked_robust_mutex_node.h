#ifndef _BITS___LOCKED_ROBUST_MUTEX_NODE_H
#define _BITS___LOCKED_ROBUST_MUTEX_NODE_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __locked_robust_mutex_node {
    int __in_progress_flags;
    int __in_progress_value;
    unsigned int *__protected;
    struct __locked_robust_mutex_node *__prev;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITS___LOCKED_ROBUST_MUTEX_NODE_H */