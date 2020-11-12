#ifndef _KERNEL_HAL_HW_TIMER_H
#define _KERNEL_HAL_HW_TIMER_H 1

#include <sys/time.h>

#include <kernel/hal/hw_device.h>

struct hw_timer;
struct irq_context;

typedef void (*hw_timer_callback_t)(struct hw_timer *timer, struct irq_context *context);

struct hw_timer_ops {
    void (*setup_interval_timer)(struct hw_timer *self, hw_timer_callback_t callback);
};

struct hw_timer {
    struct hw_device hw_device;
#define HW_TIMER_SINGLE_SHOT 1
#define HW_TIMER_INTERVAL    2
    int flags;
    struct hw_timer_ops *ops;
    struct timespec resolution;
    struct list_node list;
    hw_timer_callback_t callback;
};

struct hw_timer *create_hw_timer(const char *name, struct hw_device *parent, struct hw_device_id id, int flags, struct hw_timer_ops *ops);
void register_hw_timer(struct hw_timer *timer, struct timespec resoltuion);
void select_hw_timers(void);

struct hw_timer *hw_sched_timer(void);
struct hw_timer *hw_clock_timer(void);
struct list_node *hw_timers(void);

#endif /* _KERNEL_HAL_HW_TIMER_H */
