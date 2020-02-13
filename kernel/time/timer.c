#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

static spinlock_t id_lock = SPINLOCK_INITIALIZER;
static timer_t id_start = 0;

static struct hash_map *timer_map;

static int timer_hash(void *index, int num_buckets) {
    return *((timer_t *) index) % num_buckets;
}

static int timer_equals(void *i1, void *i2) {
    return *((timer_t *) i1) == *((timer_t *) i2);
}

static void *timer_key(void *timer) {
    return &((struct timer *) timer)->id;
}

static timer_t allocate_timerid() {
    spin_lock(&id_lock);
    timer_t ret = id_start++;
    spin_unlock(&id_lock);

#ifdef TIMER_T_ALLOCATION_DEBUG
    debug_log("Allocated timer_t: [ %d ]\n", ret);
#endif /* TIMER_T_ALLOCATION_DEBUG */

    return ret;
}

static void free_timerid(timer_t __attribute__((unused)) id) {
#ifdef TIMER_T_ALLOCATION_DEBUG
    debug_log("Freeing timer_t: [ %d ]\n", id);
#endif /* TIMER_T_ALLOCATION_DEBUG */
}

struct timer *time_get_timer(timer_t id) {
    return hash_get(timer_map, &id);
}

bool time_is_timer_armed(struct timer *timer) {
    return timer->spec.it_value.tv_sec != 0 && timer->spec.it_value.tv_nsec != 0;
}

int time_create_timer(struct clock *clock, struct sigevent *sevp, timer_t *timerid) {
    struct timer *to_add = malloc(sizeof(struct timer));
    if (!to_add) {
        return -ENOMEM;
    }

    to_add->overuns = 0;
    to_add->event = *sevp;
    to_add->id = allocate_timerid();
    to_add->next = NULL;
    to_add->prev = NULL;
    to_add->spec.it_interval.tv_sec = 0;
    to_add->spec.it_interval.tv_nsec = 0;
    to_add->spec.it_value.tv_sec = 0;
    to_add->spec.it_value.tv_nsec = 0;

    // NOTE: don't add the timer to the clock until it is armed.
    //       this way we don't update the timer when its not
    //       relevant.
    to_add->clock = clock;

    hash_put(timer_map, to_add);

    *timerid = to_add->id;
    return 0;
}

int time_delete_timer(struct timer *timer) {
    time_remove_timer_from_clock(timer->clock, timer);
    hash_del(timer_map, &timer->id);
    free_timerid(timer->id);
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

void time_tick_timer(struct timer *timer, long nanoseconds) {
    (void) timer;
    (void) nanoseconds;
}

void init_timers() {
    timer_map = hash_create_hash_map(timer_hash, timer_equals, timer_key);
}