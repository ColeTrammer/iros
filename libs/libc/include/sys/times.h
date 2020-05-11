#ifndef _SYS_TIMES_H
#define _SYS_TIMES_H 1

#include <bits/clock_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};

clock_t times(struct tms *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SYS_TIMES_H */
