#ifndef _UTIME_H
#define _UTIME_H 1

#include <bits/time_t.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct utimbuf {
    time_t actime;
    time_t modtime;
};

int utime(const char *filename, const struct utimbuf *times);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _UTIME_H */
