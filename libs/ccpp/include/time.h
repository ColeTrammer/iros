#pragma once

#include <ccpp/bits/clock_t.h>
#include <ccpp/bits/config.h>
#include <ccpp/bits/size_t.h>
#include <ccpp/bits/time_t.h>

#if defined(__CCPP_C11) || defined(__CCPP_POSIX_EXTENSIONS)
#include <ccpp/bits/timespec.h>
#endif

__CCPP_BEGIN_DECLARATIONS

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

double difftime(time_t __time_end, time_t __time_start);
time_t time(time_t* __time);

#define CLOCKS_PER_SEC 1000000
clock_t clock(void);

#ifdef __CCPP_C11
#define TIME_UTC 1
struct timespec timespec_get(struct timespec* __ts, int __base);
#endif
#ifdef __CCPP_C23
int timespec_getres(struct timespec* __ts, int __base);
#endif

__CCPP_C23_DEPRECATED char* asctime(struct tm const* __tm);
__CCPP_C23_DEPRECATED char* ctime(time_t const* __time);
size_t strftime(char* __CCPP_RESTRICT __str, size_t __count, char const* __CCPP_RESTRICT __format,
                struct tm const* __CCPP_RESTRICT __tm);

struct tm* gmtime(time_t const* __time);
#if defined(__CCPP_C23) || defined(__CCPP_POSIX_EXTENSIONS)
struct tm* gmtime_r(time_t const* __CCPP_RESTRICT __time, struct tm* __CCPP_RESTRICT __tm);
#endif
struct tm* localtime(time_t const* __time);
#if defined(__CCPP_C23) || defined(__CCPP_POSIX_EXTENSIONS)
struct tm* localtime_r(time_t const* __CCPP_RESTRICT __time, struct tm* __CCPP_RESTRICT __tm);
#endif
time_t mktime(struct tm* __tm);

__CCPP_END_DECLARATIONS
