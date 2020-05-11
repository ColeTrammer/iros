#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>

// FIXME: read this from ENV variable TZ, or do something else sensible
char *tzname[2] = { "pst", "pdt" };
long timezone = 8 * 60 * 60;
int daylight = 1;

time_t time(time_t *t_loc) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (t_loc != NULL) {
        *t_loc = tv.tv_sec;
    }

    return tv.tv_sec;
}

clock_t clock(void) {
    struct tms tms;
    times(&tms);

    return tms.tms_stime * CLOCKS_PER_SEC + tms.tms_utime;
}

size_t strftime(char *__restrict s, size_t n, const char *__restrict format, const struct tm *__restrict tm) {
    (void) s;
    (void) n;
    (void) format;
    (void) tm;

    fputs("strftime unsupported", stderr);
    return 0;
}

static char wday_name[7][3] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char mon_name[12][3] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char static_time_string_buffer[26];
static struct tm static_tm_buffer;

char *asctime(const struct tm *timeptr) {
    return asctime_r(timeptr, static_time_string_buffer);
}

char *asctime_r(const struct tm *__restrict timeptr, char *__restrict result) {
    sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n", wday_name[timeptr->tm_wday], mon_name[timeptr->tm_mon], timeptr->tm_mday,
            timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, 1900 + timeptr->tm_year);
    return result;
}

char *ctime(const time_t *timep) {
    return asctime(localtime(timep));
}

char *ctime_r(const time_t *__restrict timep, char *__restrict result) {
    struct tm tm;
    return asctime_r(localtime_r(timep, &tm), result);
}

struct tm *gmtime(const time_t *timep) {
    return gmtime_r(timep, &static_tm_buffer);
}

struct tm *gmtime_r(const time_t *__restrict timer, struct tm *__restrict result) {
    memset(result, 0, sizeof(struct tm));
    result->tm_year = 70;
    result->tm_mday = 1;
    result->tm_isdst = -daylight;

    time_t time = *timer;
    result->tm_sec = time % 60;
    result->tm_min = time / 60 % 60;
    result->tm_hour = time / 3600 % 24;
    result->tm_wday = (time / (3600 * 24) + 4) % 7; // Add 4 b/c Jan 1, 1970 was a Thursday

    time_t seconds_in_year;
    int days_in_current_year;
    do {
        days_in_current_year = result->tm_year % 4 == 0 && result->tm_year % 100 != 0 ? 366 : 365;
        seconds_in_year = days_in_current_year * 60 * 60 * 24;
    } while (time >= seconds_in_year && ((time -= seconds_in_year) || true) && result->tm_year++);

    result->tm_yday = time / (3600 * 24);

    while (time > 0) {
        int max_days = 0;
        switch (result->tm_mon + 1) {
            case 1:  // Jan
            case 3:  // Mar
            case 5:  // May
            case 7:  // Jul
            case 8:  // Aug
            case 10: // Oct
            case 12: // Dec
                max_days = 31;
                break;
            case 4:  // Apr
            case 6:  // Jun
            case 9:  // Sep
            case 11: // Nov
                max_days = 30;
                break;
            case 2:
                if (result->tm_year % 400 == 0 || (result->tm_year % 4 == 0 && result->tm_year % 100 != 0)) {
                    max_days = 29;
                } else {
                    max_days = 28;
                }
                break;
            default:
                assert(false);
        }

        if (max_days * 60 * 60 * 24 >= time) {
            result->tm_mday = time / (3600 * 24);
            break;
        }

        time -= max_days * 60 * 60 * 24;
        result->tm_mon++;
    }

    return result;
}

struct tm *localtime(const time_t *timep) {
    return localtime_r(timep, &static_tm_buffer);
}

struct tm *localtime_r(const time_t *__restrict timer, struct tm *__restrict result) {
    time_t time = *timer - timezone;

    struct tm *ret = gmtime_r(&time, result);

    // FIXME: adjust for DST if daylight == 1
    return ret;
}
