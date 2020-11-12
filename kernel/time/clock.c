#include <assert.h>
#include <search.h>
#include <stdlib.h>

#include <kernel/hal/hw_timer.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/time/timer.h>
#include <kernel/util/hash_map.h>
#include <kernel/util/init.h>
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
            clock->resolution = hw_clock_timer()->max_resolution;
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

void time_inc_clock_timers(struct list_node *timer_list, struct timespec amt, bool kernel_time) {
    list_for_each_entry_safe(timer_list, timer, struct timer, clock_list) { time_tick_timer(timer, amt, kernel_time); }
}

void __time_add_timer_to_clock(struct clock *clock, struct timer *timer) {
    list_append(&clock->timer_list, &timer->clock_list);
}

void time_add_timer_to_clock(struct clock *clock, struct timer *timer) {
    spin_lock(&clock->lock);
    __time_add_timer_to_clock(clock, timer);
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

static void __inc_global_clocks(struct hw_timer_channel *channel) {
    time_inc_clock(&global_monotonic_clock, channel->interval, false);
    time_inc_clock(&global_realtime_clock, channel->interval, false);
}

extern uint64_t idle_ticks;
extern uint64_t user_ticks;
extern uint64_t kernel_ticks;

static void on_hw_sched_tick(struct hw_timer_channel *channel, struct irq_context *context) {
    struct task *current = get_current_task();
    if (current == get_idle_task()) {
        idle_ticks++;
    } else if (current->in_kernel) {
        current->process->rusage_self.ru_stime =
            time_add_timeval(current->process->rusage_self.ru_stime, timeval_from_time(channel->interval));
        kernel_ticks++;
    } else {
        current->process->rusage_self.ru_utime =
            time_add_timeval(current->process->rusage_self.ru_utime, timeval_from_time(channel->interval));
        user_ticks++;
    }
    // Check for NULL b/c kernel tasks don't have a clock
    if (current->task_clock) {
        time_inc_clock(current->task_clock, channel->interval, current->in_kernel);
        time_inc_clock(current->process->process_clock, channel->interval, current->in_kernel);
    }

    if (atomic_load(&current->process->should_profile)) {
        // To seriously support profiling multiple threads, the buffer and lock should be per-thread and not per-process.
        spin_lock(&current->process->profile_buffer_lock);
        // Make sure not to write into a stale buffer.
        if (current->process->profile_buffer) {
            proc_record_profile_stack(context->task_state);
        }
        spin_unlock(&current->process->profile_buffer_lock);
    }

    sched_tick(context->task_state);
}

static void on_hw_clock_tick(struct hw_timer_channel *channel, struct irq_context *context) {
    (void) context;
    __inc_global_clocks(channel);
}

static void init_clocks() {
    clock_map = hash_create_hash_map(clock_hash, clock_equals, clock_key);

    select_hw_timers();
    struct hw_timer *clock_timer = hw_clock_timer();
    assert(clock_timer);

    clock_timer->ops->setup_interval_timer(clock_timer, 0, on_hw_clock_tick);

    global_monotonic_clock.resolution = clock_timer->channels[0].interval;
    global_realtime_clock.resolution = clock_timer->channels[0].interval;

    struct hw_timer *sched_timer = hw_sched_timer();
    assert(sched_timer);

    sched_timer->ops->setup_interval_timer(sched_timer, 0, on_hw_sched_tick);
}
INIT_FUNCTION(init_clocks, time);
