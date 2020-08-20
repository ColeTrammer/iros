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

void time_inc_clock_timers(struct list_node *timer_list, long nanoseconds);
void __time_add_timer_to_clock(struct clock *clock, struct timer *timer);
void time_add_timer_to_clock(struct clock *clock, struct timer *timer);
void __time_remove_timer_from_clock(struct clock *clock, struct timer *timer);
void time_remove_timer_from_clock(struct clock *clock, struct timer *timer);

void init_clocks();

static inline __attribute__((always_inline)) void time_inc_clock(struct clock *clock, long nanoseconds) {
    spin_lock(&clock->lock);
    clock->time.tv_nsec += nanoseconds;
    if (clock->time.tv_nsec >= 1000000000L) {
        clock->time.tv_nsec %= 1000000000L;
        clock->time.tv_sec++;
    }

    time_inc_clock_timers(&clock->timer_list, nanoseconds);
    spin_unlock(&clock->lock);
}

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

static inline __attribute__((always_inline)) struct timespec time_sub(struct timespec t1, struct timespec t2) {
    t1.tv_sec -= t2.tv_sec;
    t1.tv_nsec -= t2.tv_nsec;
    if (t1.tv_nsec < 0) {
        t1.tv_nsec += 1000000000L;
        t1.tv_sec--;
    }

    return t1;
}

static inline __attribute__((always_inline)) struct timespec time_from_timeval(struct timeval v) {
    return (struct timespec) { .tv_sec = v.tv_sec, .tv_nsec = v.tv_usec * 1000 };
}

extern struct clock global_monotonic_clock;
extern struct clock global_realtime_clock;

#endif /* _KERNEL_TIME_CLOCK_H */
