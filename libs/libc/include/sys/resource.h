#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H 1

#include <bits/id_t.h>
#include <sys/time.h>

#define PRIO_PROCESS 0
#define PRIO_PGRP    1
#define PRIO_USER    2

#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN 1

#define RLIMIT_CORE   0
#define RLIMIT_CPU    1
#define RLIMIT_DATA   2
#define RLIMIT_FSIZE  3
#define RLIMIT_NOFILE 4
#define RLIMIT_STACK  5
#define RLIMIT_AS     6

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned long rlim_t;

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
};

int getpriority(int flag, id_t who);
int getrlimit(int what, struct rlimit *res);
int getrusage(int who, struct rusage *res);
int setprioity(int flag, id_t who, int val);
int setrlimit(int what, const struct rlimit *to_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define RLIM_INFINITY  ((rlim_t) -1)
#define RLIM_SAVED_MAX ((rlim_t) -2)
#define RLIM_SAVED_CUR ((rlim_t) -3)

#endif /* _SYS_RESOURCE_H */