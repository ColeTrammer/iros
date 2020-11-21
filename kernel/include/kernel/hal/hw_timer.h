#ifndef _KERNEL_HAL_HW_TIMER_H
#define _KERNEL_HAL_HW_TIMER_H 1

#include <sys/time.h>

#include <kernel/hal/hw_device.h>
#include <kernel/irqs/handlers.h>

struct hw_timer_channel;
struct hw_timer;

typedef void (*hw_timer_callback_t)(struct hw_timer_channel *timer, struct irq_context *context);

struct hw_timer_ops {
    void (*setup_interval_timer)(struct hw_timer *self, int channel_index, long frequency, hw_timer_callback_t callback);
    void (*setup_one_shot_timer)(struct hw_timer *self, int channel_index, struct timespec delay, hw_timer_callback_t callback);
    void (*disable_channel)(struct hw_timer *self, int channel_index);
    void (*calibrate)(struct hw_timer *self, struct hw_timer *reference);
};

struct hw_timer_channel {
    struct irq_handler irq_handler;
    struct timespec interval;
    long frequency;
    hw_timer_callback_t callback;
    struct hw_timer *timer;
    int type : 31;
    int valid : 1;
};

struct hw_timer {
    struct hw_device hw_device;
#define HW_TIMER_SINGLE_SHOT       1
#define HW_TIMER_INTERVAL          2
#define HW_TIMER_PER_CPU           4
#define HW_TIMER_HAS_COUNTER       8
#define HW_TIMER_NEEDS_CALIBRATION 16
    int flags;
    struct hw_timer_ops *ops;
    long base_frequency;
    struct timespec max_resolution;
    struct list_node list;
    size_t num_channels;
    struct hw_timer_channel channels[0];
};

static inline int hw_timer_channel_index(struct hw_timer_channel *channel) {
    int ret = channel - channel->timer->channels;
    assert(ret >= 0 && ret < (int) channel->timer->num_channels);
    return ret;
}

struct hw_timer *create_hw_timer(const char *name, struct hw_device *parent, struct hw_device_id id, int flags, long base_frequency,
                                 struct hw_timer_ops *ops, size_t num_channels);
void register_hw_timer(struct hw_timer *timer);
void select_hw_timers(void);
int show_hw_timer_channel(struct hw_timer_channel *channel, char *buffer, size_t buffer_length);
int show_hw_timer(struct hw_timer *timer, char *buffer, size_t buffer_length);

void init_hw_timer_channel(struct hw_timer_channel *channel, irq_function_t irq_function, int irq_flags, struct hw_timer *timer, int type,
                           long frequency, hw_timer_callback_t callback);
void destroy_hw_timer_channel(struct hw_timer_channel *channel);

struct hw_timer *hw_sched_timer(void);
struct hw_timer *hw_clock_timer(void);
struct hw_timer *hw_profile_timer(void);
struct hw_timer *hw_reference_timer(void);
struct list_node *hw_timers(void);

#endif /* _KERNEL_HAL_HW_TIMER_H */
