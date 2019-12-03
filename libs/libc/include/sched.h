#ifndef _SCHED_H
#define _SCHED_H 1

#include <time.h>

#define SCHED_FIFO     1
#define SCHED_RR       2
#define SCHED_SPORADIC 3
#define SCHED_OTHER    4

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct sched_param {
    int sched_priority;
    int sched_ss_low_priority;
    int sched_ss_max_repl;
    struct timespec sched_ss_repl_period;
    struct timespec sched_ss_init_budget;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SCHED_H */