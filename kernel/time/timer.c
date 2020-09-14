#include <assert.h>
#include <errno.h>
#include <search.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <kernel/hal/processor.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

// #define TIMER_DEBUG
// #define TIMER_T_ALLOCATION_DEBUG

static spinlock_t id_lock = SPINLOCK_INITIALIZER;
static timer_t id_start = 1;

static struct hash_map *timer_map;

HASH_DEFINE_FUNCTIONS(timer, struct timer, timer_t, id)

static timer_t allocate_timerid() {
    spin_lock(&id_lock);
    timer_t ret = id_start++;
    spin_unlock(&id_lock);

#ifdef TIMER_T_ALLOCATION_DEBUG
    debug_log("Allocated timer_t: [ %ld ]\n", ret);
#endif /* TIMER_T_ALLOCATION_DEBUG */

    return ret;
}

static void free_timerid(timer_t __attribute__((unused)) id) {
#ifdef TIMER_T_ALLOCATION_DEBUG
    debug_log("Freeing timer_t: [ %ld ]\n", id);
#endif /* TIMER_T_ALLOCATION_DEBUG */
}

struct timer *time_get_timer(timer_t id) {
    return hash_get_entry(timer_map, &id, struct timer);
}

bool time_is_timer_armed(struct timer *timer) {
    return timer->spec.it_value.tv_sec != 0 || timer->spec.it_value.tv_nsec != 0;
}

int time_create_timer(struct clock *clock, struct sigevent *sevp, timer_t *timerid) {
    struct timer *to_add = malloc(sizeof(struct timer));
    if (!to_add) {
        return -EAGAIN;
    }

    to_add->overuns = 0;
    to_add->event_type = sevp ? sevp->sigev_notify : SIGEV_SIGNAL;
    to_add->id = allocate_timerid();
    to_add->spec.it_interval.tv_sec = 0;
    to_add->spec.it_interval.tv_nsec = 0;
    to_add->spec.it_value.tv_sec = 0;
    to_add->spec.it_value.tv_nsec = 0;
    init_list(&to_add->clock_list);

    // NOTE: don't add the timer to the clock until it is armed.
    //       this way we don't update the timer when its not
    //       relevant.
    to_add->clock = clock;
    to_add->task = get_current_task();

    if (to_add->event_type == SIGEV_SIGNAL || to_add->event_type == SIGEV_THREAD) {
        to_add->signal = calloc(1, sizeof(struct queued_signal));
        if (!to_add->signal) {
            free(to_add);
            return -EAGAIN;
        }

        to_add->signal->info.si_code = SI_TIMER;

        if (sevp) {
            to_add->signal->info.si_value = sevp->sigev_value;
            to_add->signal->info.si_signo = sevp->sigev_signo;
        } else {
            to_add->signal->info.si_value.sival_int = to_add->id;
            to_add->signal->info.si_signo = SIGALRM;
        }
    }

    struct process *process = to_add->task->process;
    list_append(&process->timer_list, &to_add->proc_list);

    hash_put(timer_map, &to_add->hash);
#ifdef TIMER_DEBUG
    debug_log("Created timer: [ %ld ]\n", to_add->id);
#endif /* TIMER_DEBUG */

    *timerid = to_add->id;
    return 0;
}

int time_delete_timer(struct timer *timer) {
#ifdef TIMER_DEBUG
    debug_log("Removing timer: [ %ld ]\n", timer->id);
#endif /* TIMER_DEBUG */

    if (time_is_timer_armed(timer)) {
        time_remove_timer_from_clock(timer->clock, timer);
    }

    if (timer->signal) {
        task_free_queued_signal(timer->signal);
        timer->signal = NULL;
    }

    // Kernel timers don't have an ID, and instead of being own by a process, are managed by
    // dedicated kernel data structures.
    if (timer->id) {
        list_remove(&timer->proc_list);
        hash_del(timer_map, &timer->id);
        free_timerid(timer->id);
    }

    free(timer);
    return 0;
}

int time_get_timer_overrun(struct timer *timer) {
    return timer->overuns;
}

int time_get_timer_value(struct timer *timer, struct itimerspec *valp) {
    *valp = timer->spec;
    return 0;
}

int time_set_timer(struct timer *timer, int flags, const struct itimerspec *new_spec, struct itimerspec *old) {
    (void) flags;

    if (old) {
        *old = timer->spec;
    }

    if (!new_spec) {
        return 0;
    }

    bool was_armed = time_is_timer_armed(timer);
    timer->spec = *new_spec;

    bool now_armed = time_is_timer_armed(timer);

    if (was_armed != now_armed) {
        if (now_armed) {
            time_add_timer_to_clock(timer->clock, timer);
        } else {
            time_remove_timer_from_clock(timer->clock, timer);
        }
    }

    return 0;
}

struct timer *time_register_kernel_callback(struct timespec *delay, void (*callback)(struct timer *timer, void *closure), void *closure) {
    struct timer *timer = calloc(1, sizeof(struct timer));
    timer->event_type = SIGEV_KERNEL;
    timer->clock = time_get_clock(CLOCK_MONOTONIC);
    timer->spec.it_value = *delay;
    timer->kernel_callback = callback;
    timer->kernel_callback_closure = closure;
    time_add_timer_to_clock(timer->clock, timer);
    return timer;
}

// This function is intended to be called from within the timer callback, when the corresponding
// timer's clock's lock is already held.
void __time_reset_kernel_callback(struct timer *timer, struct timespec *new_delay) {
    timer->spec.it_value = *new_delay;
    __time_add_timer_to_clock(timer->clock, timer);
}

void time_reset_kernel_callback(struct timer *timer, struct timespec *new_delay) {
    struct itimerspec spec = { .it_value = *new_delay, .it_interval = { 0 } };
    time_set_timer(timer, 0, &spec, NULL);
}

void time_cancel_kernel_callback(struct timer *timer) {
    time_delete_timer(timer);
}

void time_fire_timer(struct timer *timer) {
    switch (timer->event_type) {
        case SIGEV_SIGNAL:
        case SIGEV_THREAD:
            // If already allocated, update overrun count.
            if (timer->signal->flags & QUEUED_SIGNAL_DONT_FREE_FLAG) {
                timer->overuns++;
            } else {
                // The flag will be queued everytime the signal is dequeued
                // so that its status can be known
                timer->signal->flags |= QUEUED_SIGNAL_DONT_FREE_FLAG;
                task_enqueue_signal_object(timer->task, timer->signal);
            }
            break;
        case SIGEV_KERNEL:
            assert(timer->kernel_callback);
            timer->kernel_callback(timer, timer->kernel_callback_closure);
            break;
        case SIGEV_NONE:
            break;
        default:
            assert(false);
            break;
    }
}

void time_tick_timer(struct timer *timer, long nanoseconds, bool kernel_time) {
    if (timer->ignore_kernel_ticks && kernel_time) {
        return;
    }

    struct timespec to_sub = { .tv_sec = nanoseconds / INT64_C(1000000000), .tv_nsec = nanoseconds % INT64_C(1000000000) };
    timer->spec.it_value = time_sub(timer->spec.it_value, to_sub);

    // This means timer expired
    if (timer->spec.it_value.tv_sec < 0 || (timer->spec.it_value.tv_sec == 0 && timer->spec.it_value.tv_nsec == 0)) {
        if (timer->spec.it_interval.tv_sec != 0 || timer->spec.it_interval.tv_nsec != 0) {
            do {
                time_fire_timer(timer);
                timer->spec.it_value = time_add(timer->spec.it_value, timer->spec.it_interval);
            } while (timer->spec.it_value.tv_sec < 0 || (timer->spec.it_value.tv_sec == 0 && timer->spec.it_value.tv_nsec == 0));
        } else {
            timer->spec.it_value.tv_sec = timer->spec.it_value.tv_nsec = 0;
            __time_remove_timer_from_clock(timer->clock, timer);
            time_fire_timer(timer);
        }
    }
}

int time_getitimer(int which, struct itimerval *valp) {
    int ret = 0;
    struct timer *timer = NULL;
    struct process *current = get_current_process();

    mutex_lock(&current->lock);
    switch (which) {
        case ITIMER_REAL:
            timer = current->alarm_timer;
            break;
        case ITIMER_PROF:
            timer = current->profile_timer;
            break;
        case ITIMER_VIRTUAL:
            timer = current->virtual_timer;
            break;
        default:
            ret = -EINVAL;
            goto done;
    }

    struct itimerval val = { 0 };
    if (timer) {
        struct itimerspec valspec;
        if ((ret = time_get_timer_value(timer, &valspec))) {
            goto done;
        }
        val = itimerval_from_itimerspec(valspec);
    }
    *valp = val;

done:
    mutex_unlock(&current->lock);
    return ret;
}

int time_setitimer(int which, const struct itimerval *nvalp, struct itimerval *ovalp) {
    if (!nvalp && !ovalp) {
        return -EINVAL;
    }

    int ret = 0;
    struct timer **timerp;
    struct process *current = get_current_process();

    switch (which) {
        case ITIMER_REAL:
            timerp = &current->alarm_timer;
            break;
        case ITIMER_PROF:
            timerp = &current->profile_timer;
            break;
        case ITIMER_VIRTUAL:
            timerp = &current->virtual_timer;
            break;
        default:
            return -EINVAL;
    }

    mutex_lock(&current->lock);
    if (!*timerp) {
        struct clock *clock;
        struct sigevent sev = { .sigev_notify = SIGEV_SIGNAL };
        switch (which) {
            case ITIMER_REAL:
                clock = time_get_clock(CLOCK_REALTIME);
                sev.sigev_signo = SIGALRM;
                break;
            case ITIMER_PROF:
                clock = current->process_clock;
                sev.sigev_signo = SIGPROF;
                break;
            case ITIMER_VIRTUAL:
                clock = current->process_clock;
                sev.sigev_signo = SIGVTALRM;
                break;
        }

        timer_t id;
        if ((ret = time_create_timer(clock, &sev, &id))) {
            goto done;
        }
        *timerp = time_get_timer(id);
        (*timerp)->ignore_kernel_ticks = which == ITIMER_VIRTUAL;
    }

    struct timer *timer = *timerp;
    if (ovalp) {
        *ovalp = itimerval_from_itimerspec(timer->spec);
    }

    if (nvalp) {
        struct itimerspec spec = itimerspec_from_itimerval(*nvalp);
        ret = time_set_timer(timer, 0, &spec, NULL);
    }

done:
    mutex_unlock(&current->lock);
    return ret;
}

void init_timers() {
    timer_map = hash_create_hash_map(timer_hash, timer_equals, timer_key);
}
