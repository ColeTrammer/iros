#ifndef _KERNEL_TIME_CLOCK_H
#define _KERNEL_TIME_CLOCK_H 1

#include <sys/time.h>
#include <time.h>

#include <kernel/util/hash_map.h>
#include <kernel/util/list.h>
#include <kernel/util/spinlock.h>

struct timer;

struct clock {
    clockid_t id;
    spinlock_t lock;
    struct timespec resolution;
    struct timespec time;
    struct list_node timer_list;
    struct hash_entry hash;
};

struct clock *time_create_clock(clockid_t id);
void time_destroy_clock(struct clock *clock);

struct clock *time_get_clock(clockid_t id);
struct timespec time_read_clock(clockid_t id);

void time_inc_clock_timers(struct list_node *timer_list, struct timespec amt, bool kernel_time);
void __time_add_timer_to_clock(struct clock *clock, struct timer *timer);
void time_add_timer_to_clock(struct clock *clock, struct timer *timer);
void __time_remove_timer_from_clock(struct clock *clock, struct timer *timer);
void time_remove_timer_from_clock(struct clock *clock, struct timer *timer);

static inline __attribute__((always_inline)) long time_compare(struct timespec t1, struct timespec t2) {
    if (t1.tv_sec == t2.tv_sec) {
        return t1.tv_nsec - t2.tv_nsec;
    }

    return t1.tv_sec - t2.tv_sec;
}

static inline __attribute__((always_inline)) struct timespec time_add(struct timespec t1, struct timespec t2) {
    t1.tv_sec += t2.tv_sec;
    t1.tv_nsec += t2.tv_nsec;
    if (t1.tv_nsec >= 1000000000L) {
        t1.tv_nsec %= 1000000000L;
        t1.tv_sec++;
    }

    return t1;
}

static inline __attribute__((always_inline)) struct timeval time_add_timeval(struct timeval t1, struct timeval t2) {
    t1.tv_sec += t2.tv_sec;
    t1.tv_usec += t2.tv_usec;
    if (t1.tv_usec >= 1000000L) {
        t1.tv_usec %= 1000000L;
        t1.tv_sec++;
    }

    return t1;
}

static inline __attribute__((always_inline)) struct timespec time_sub(struct timespec t1, struct timespec t2) {
    t1.tv_sec -= t2.tv_sec;
    t1.tv_nsec -= t2.tv_nsec;
    if (t1.tv_nsec < 0) {
        t1.tv_nsec += 1000000000L;
        t1.tv_sec--;
    }

    return t1;
}

static inline __attribute__((always_inline)) long time_divide(struct timespec time, long frequency) {
    long a = time.tv_sec * 1000000000 + time.tv_nsec;
    return ALIGN_UP(a, frequency) / frequency;
}

static inline __attribute__((always_inline)) void time_inc_clock(struct clock *clock, struct timespec amt, bool kernel_time) {
    spin_lock(&clock->lock);
    clock->time = time_add(clock->time, amt);
    time_inc_clock_timers(&clock->timer_list, amt, kernel_time);
    spin_unlock(&clock->lock);
}

static inline __attribute__((always_inline)) struct timespec time_from_timeval(struct timeval v) {
    return (struct timespec) { .tv_sec = v.tv_sec, .tv_nsec = v.tv_usec * 1000 };
}

static inline __attribute__((always_inline)) struct timeval timeval_from_time(struct timespec s) {
    return (struct timeval) { .tv_sec = s.tv_sec, .tv_usec = s.tv_nsec / 1000 };
}

static inline __attribute__((always_inline)) struct itimerspec itimerspec_from_itimerval(struct itimerval v) {
    return (struct itimerspec) { .it_value = time_from_timeval(v.it_value), .it_interval = time_from_timeval(v.it_interval) };
}

static inline __attribute__((always_inline)) struct itimerval itimerval_from_itimerspec(struct itimerspec s) {
    return (struct itimerval) { .it_value = timeval_from_time(s.it_value), .it_interval = timeval_from_time(s.it_interval) };
}

extern struct clock global_monotonic_clock;
extern struct clock global_realtime_clock;

#endif /* _KERNEL_TIME_CLOCK_H */
