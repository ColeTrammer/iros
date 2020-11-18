#include <stdlib.h>
#include <string.h>

#include <kernel/hal/hw_timer.h>

static struct list_node s_hw_timers = INIT_LIST(s_hw_timers);
static struct hw_timer *s_hw_sched_timer;
static struct hw_timer *s_hw_clock_timer;

struct hw_timer *create_hw_timer(const char *name, struct hw_device *parent, struct hw_device_id id, int flags, long base_frequency,
                                 struct hw_timer_ops *ops, size_t num_channels) {
    struct hw_timer *hw_timer = calloc(1, sizeof(struct hw_timer) + num_channels * sizeof(struct hw_timer_channel));
    hw_timer->flags = flags;
    hw_timer->ops = ops;
    hw_timer->num_channels = num_channels;
    hw_timer->base_frequency = base_frequency;
    if (base_frequency != 0) {
        hw_timer->max_resolution = (struct timespec) { .tv_nsec = 1000000000 / base_frequency };
    }
    init_hw_device(&hw_timer->hw_device, name, parent, id, NULL, NULL);
    return hw_timer;
}

int show_hw_timer_channel(struct hw_timer_channel *channel, char *buffer, size_t _buffer_length) {
    int position = 0;
    int buffer_length = _buffer_length;

    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "    VALID: %d\n", !!channel->valid);
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "    TYPE: %s\n",
                         channel->type == HW_TIMER_INTERVAL ? "interval" : "single_shot");
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "    FREQUENCY: %ld\n", channel->frequency);
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "    INTERVAL_S: %ld\n", channel->interval.tv_sec);
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "    INTERVAL_NS: %ld\n", channel->interval.tv_nsec);

    return position;
}

int show_hw_timer(struct hw_timer *timer, char *buffer, size_t _buffer_length) {
    int position = 0;
    int buffer_length = _buffer_length;
    char aux_buffer[512];

    show_hw_device(&timer->hw_device, aux_buffer, sizeof(aux_buffer));
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "%s\n", aux_buffer);

    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "  SCHED_TIMER: %d\n", timer == hw_sched_timer());
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "  CLOCK_TIMER: %d\n", timer == hw_clock_timer());
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "  BASE_FREQUENCY: %ld\n", timer->base_frequency);
    position += snprintf(buffer + position, MAX(buffer_length - position, 0), "  MAX_PRECISION_NS: %ld\n", timer->max_resolution.tv_nsec);
    position +=
        snprintf(buffer + position, MAX(buffer_length - position, 0), "  FLAGS: single_shot=%d interval=%d per_cpu=%d has_counter=%d\n",
                 !!(timer->flags & HW_TIMER_SINGLE_SHOT), !!(timer->flags & HW_TIMER_INTERVAL), !!(timer->flags & HW_TIMER_PER_CPU),
                 !!(timer->flags & HW_TIMER_HAS_COUNTER));

    for (size_t i = 0; i < timer->num_channels; i++) {
        show_hw_timer_channel(&timer->channels[i], aux_buffer, sizeof(aux_buffer));
        position += snprintf(buffer + position, MAX(buffer_length - position, 0), "  CHANNEL: %lu\n%s", i, aux_buffer);
    }

    return position;
}

void register_hw_timer(struct hw_timer *timer) {
    list_append(&s_hw_timers, &timer->list);
}

void init_hw_timer_channel(struct hw_timer_channel *channel, irq_function_t irq_function, int irq_flags, struct hw_timer *timer, int type,
                           long frequency, hw_timer_callback_t callback) {
    channel->irq_handler.closure = channel;
    channel->irq_handler.flags = irq_flags;
    channel->irq_handler.handler = irq_function;

    channel->callback = callback;
    channel->frequency = frequency;
    channel->interval = (struct timespec) { .tv_nsec = 1000000000 / frequency };
    channel->timer = timer;
    channel->type = type;
    channel->valid = 1;
}

void destroy_hw_timer_channel(struct hw_timer_channel *channel) {
    memset(channel, 0, sizeof(struct hw_timer_channel));
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
