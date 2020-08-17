#include <assert.h>
#include <search.h>
#include <stdlib.h>

#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/timer.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/spinlock.h>

// #define CLOCKID_ALLOCATION_DEBUG

struct clock global_monotonic_clock = {
    CLOCK_MONOTONIC, SPINLOCK_INITIALIZER, { 0 }, { 0 }, INIT_LIST(global_monotonic_clock.timer_list), { 0 },
};
struct clock global_realtime_clock = {
    CLOCK_REALTIME, SPINLOCK_INITIALIZER, { 0 }, { 0 }, INIT_LIST(global_realtime_clock.timer_list), { 0 },
};

static spinlock_t id_lock = SPINLOCK_INITIALIZER;
static clockid_t id_start = 10; // Start at 10 since CLOCK_MONOTONIC etc. are reserved

static struct hash_map *clock_map;

HASH_DEFINE_FUNCTIONS(clock, struct clock, clockid_t, id)

static clockid_t allocate_clockid() {
    spin_lock(&id_lock);
    clockid_t ret = id_start++;
    spin_unlock(&id_lock);

#ifdef CLOCKID_ALLOCATION_DEBUG
    debug_log("Allocated clockid: [ %d ]\n", ret);
#endif /* CLOCKID_ALLOCATION_DEBUG */

    return ret;
}

static void free_clockid(clockid_t __attribute__((unused)) id) {
#ifdef CLOCKID_ALLOCATION_DEBUG
    debug_log("Freeing clockid: [ %d ]\n", id);
#endif /* CLOCKID_ALLOCATION_DEBUG */
}

struct clock *time_create_clock(clockid_t id) {
    struct clock *clock;

    switch (id) {
        case CLOCK_PROCESS_CPUTIME_ID:
        case CLOCK_THREAD_CPUTIME_ID: {
            clockid_t real_id = allocate_clockid();
            clock = malloc(sizeof(struct clock));
            clock->id = real_id;
            clock->resolution = get_time_resolution();
            clock->time = (struct timespec) { 0 };
            init_list(&clock->timer_list);
            init_spinlock(&clock->lock);
            break;
        }
        default:
            return NULL;
    }

    hash_put(clock_map, &clock->hash);
    return clock;
}

void time_destroy_clock(struct clock *clock) {
    free_clockid(clock->id);
    hash_del(clock_map, &clock->id);
    free(clock);
}

struct clock *time_get_clock(clockid_t id) {
    switch (id) {
        case CLOCK_MONOTONIC:
            return &global_monotonic_clock;
        case CLOCK_REALTIME:
            return &global_realtime_clock;
        case CLOCK_PROCESS_CPUTIME_ID:
            return get_current_task()->process->process_clock;
        case CLOCK_THREAD_CPUTIME_ID:
            return get_current_task()->task_clock;
        default:
            break;
    }
    return hash_get_entry(clock_map, &id, struct clock);
}

struct timespec time_read_clock(clockid_t id) {
    struct clock *clock = time_get_clock(id);
    assert(clock);
    return clock->time;
}

void time_inc_clock_timers(struct list_node *timer_list, long nanoseconds) {
    list_for_each_entry_safe(timer_list, timer, struct timer, clock_list) { time_tick_timer(timer, nanoseconds); }
}

void time_add_timer_to_clock(struct clock *clock, struct timer *timer) {
    spin_lock(&clock->lock);
    list_append(&clock->timer_list, &timer->clock_list);
    spin_unlock(&clock->lock);
}

void __time_remove_timer_from_clock(struct clock *clock __attribute__((unused)), struct timer *timer) {
    list_remove(&timer->clock_list);
}

void time_remove_timer_from_clock(struct clock *clock, struct timer *timer) {
    spin_lock(&clock->lock);
    __time_remove_timer_from_clock(clock, timer);
    spin_unlock(&clock->lock);
}

static void __inc_global_clocks() {
    time_inc_clock(&global_monotonic_clock, 1000000);
    time_inc_clock(&global_realtime_clock, 1000000);
}

void init_clocks() {
    clock_map = hash_create_hash_map(clock_hash, clock_equals, clock_key);
    global_monotonic_clock.resolution = get_time_resolution();
    global_realtime_clock.resolution = get_time_resolution();

    register_callback(__inc_global_clocks, 1);
}
