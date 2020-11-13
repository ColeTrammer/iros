#include <stdlib.h>

#include <kernel/hal/hw_timer.h>

static struct list_node s_hw_timers = INIT_LIST(s_hw_timers);
static struct hw_timer *s_hw_sched_timer;
static struct hw_timer *s_hw_clock_timer;

struct hw_timer *create_hw_timer(const char *name, struct hw_device *parent, struct hw_device_id id, int flags,
                                 struct timespec max_resolution, struct hw_timer_ops *ops, size_t num_channels) {
    struct hw_timer *hw_timer = calloc(1, sizeof(struct hw_timer) + num_channels * sizeof(struct hw_timer_channel));
    hw_timer->flags = flags;
    hw_timer->ops = ops;
    hw_timer->num_channels = num_channels;
    hw_timer->max_resolution = max_resolution;
    init_hw_device(&hw_timer->hw_device, name, parent, id, NULL, NULL);
    return hw_timer;
}

int show_hw_timer(struct hw_timer *timer, char *buffer, size_t buffer_length) {
    return snprintf(buffer, buffer_length, "%s\n", timer->hw_device.name);
}

void register_hw_timer(struct hw_timer *timer) {
    list_append(&s_hw_timers, &timer->list);
}

void init_hw_timer_channel(struct hw_timer_channel *channel, irq_function_t irq_function, int irq_flags, struct hw_timer *timer, int type,
                           struct timespec interval, hw_timer_callback_t callback) {
    channel->irq_handler.closure = channel;
    channel->irq_handler.flags = irq_flags;
    channel->irq_handler.handler = irq_function;

    channel->callback = callback;
    channel->interval = interval;
    channel->timer = timer;
    channel->type = type;
    channel->valid = 1;
}

void select_hw_timers(void) {
    list_for_each_entry(&s_hw_timers, timer, struct hw_timer, list) {
        if (timer->flags & HW_TIMER_SINGLE_SHOT) {
            debug_log("Selected scheduler timer: [ %s ]\n", timer->hw_device.name);
            s_hw_sched_timer = timer;
            break;
        }
    }

    list_for_each_entry(&s_hw_timers, timer, struct hw_timer, list) {
        if (timer != s_hw_sched_timer && !!(timer->flags & HW_TIMER_INTERVAL)) {
            debug_log("Selected clock timer: [ %s ]\n", timer->hw_device.name);
            s_hw_clock_timer = timer;
            break;
        }
    }
}

struct hw_timer *hw_sched_timer(void) {
    return s_hw_sched_timer;
}

struct hw_timer *hw_clock_timer(void) {
    return s_hw_clock_timer;
}

struct list_node *hw_timers(void) {
    return &s_hw_timers;
}
