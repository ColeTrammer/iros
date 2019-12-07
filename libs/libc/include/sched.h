#ifndef _SCHED_H
#define _SCHED_H 1

#include <bits/__sched_param.h>
#include <time.h>

// aligned this way for the pthread_attr_t structure
#define SCHED_FIFO     4
#define SCHED_RR       5
#define SCHED_SPORADIC 6
#define SCHED_OTHER    0
#define __SCHED_MASK   12

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef __sched_param_defined
#define __sched_param_defined 1

#define sched_param __sched_param

#endif /* __sched_param_defined */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SCHED_H */