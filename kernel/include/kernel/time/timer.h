#ifndef _KERNEL_TIME_TIMER_H
#define _KERNEL_TIME_TIMER_H 1

#include <signal.h>
#include <stdbool.h>
#include <time.h>

struct clock;
struct queued_signal;
struct task;

struct timer {
    // Linked list between timer waiting on same clock
    struct timer *next;
    struct timer *prev;
    struct clock *clock;

    struct itimerspec spec;
    int event_type;
    int overuns;
    timer_t id;
    struct queued_signal *signal;
    struct task *task;
};

struct timer *time_get_timer(timer_t id);

bool time_is_timer_armed(struct timer *timer);

int time_create_timer(struct clock *clock, struct sigevent *sevp, timer_t *timerid);
int time_delete_timer(struct timer *timer);
int time_get_timer_overrun(struct timer *timer);
int time_get_timer_value(struct timer *timer, struct itimerspec *valp);
int time_set_timer(struct timer *timer, int flags, const struct itimerspec *new_spec, struct itimerspec *old);

void time_fire_timer(struct timer *timer);
void time_tick_timer(struct timer *timer, long nanoseconds);

void init_timers();

#endif /* _KERNEL_TIME_TIMER_H */