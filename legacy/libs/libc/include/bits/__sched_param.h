#ifndef ____sched_param_defined
#define ____sched_param_defined 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct __sched_param {
    int sched_priority;
    // int sched_ss_low_priority;
    // int sched_ss_max_repl;
    // struct timespec sched_ss_repl_period;
    // struct timespec sched_ss_init_budget;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __sched_param_defined */
