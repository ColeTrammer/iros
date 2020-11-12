#ifndef _KERNEL_TIME_TIMER_H
#define _KERNEL_TIME_TIMER_H 1

#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include <kernel/util/hash_map.h>
#include <kernel/util/list.h>

struct clock;
struct queued_signal;
struct task;

struct timer {
    // Linked list between timers waiting on same clock
    struct list_node clock_list;

    // Linked list between timers of same process
    struct list_node proc_list;

    struct hash_entry hash;

    struct clock *clock;

    void (*kernel_callback)(struct timer *timer, void *closure);
    void *kernel_callback_closure;

    struct itimerspec spec;
    int event_type;
    int overuns;
    timer_t id;
    struct queued_signal *signal;
    struct task *task;
    bool ignore_kernel_ticks : 1;
};

struct timer *time_get_timer(timer_t id);

bool time_is_timer_armed(struct timer *timer);

int time_create_timer(struct clock *clock, struct sigevent *sevp, timer_t *timerid);
int time_delete_timer(struct timer *timer);
int time_get_timer_overrun(struct timer *timer);
int time_get_timer_value(struct timer *timer, struct itimerspec *valp);
int time_set_timer(struct timer *timer, int flags, const struct itimerspec *new_spec, struct itimerspec *old);

int time_getitimer(int which, struct itimerval *valp);
int time_setitimer(int which, const struct itimerval *nvalp, struct itimerval *ovalp);

int time_wakeup_after(int clockid, struct timespec *time);

struct timer *time_register_kernel_callback(struct timespec *delay, void (*callback)(struct timer *timer, void *closure), void *closure);
void __time_reset_kernel_callback(struct timer *timer, struct timespec *new_delay);
void time_reset_kernel_callback(struct timer *timer, struct timespec *new_delay);
void time_cancel_kernel_callback(struct timer *timer);

void time_fire_timer(struct timer *timer);
void time_tick_timer(struct timer *timer, struct timespec amt, bool kernel_time);

#endif /* _KERNEL_TIME_TIMER_H */
