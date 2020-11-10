#include <stdlib.h>

#include <kernel/hal/hw_timer.h>

static struct list_node s_hw_timers = INIT_LIST(s_hw_timers);

struct hw_timer *create_hw_timer(const char *name, struct hw_device *parent, struct hw_device_id id, int flags, struct hw_timer_ops *ops) {
    struct hw_timer *hw_timer = calloc(1, sizeof(struct hw_timer));
    hw_timer->flags = flags;
    hw_timer->ops = ops;
    init_hw_device(&hw_timer->hw_device, name, parent, id, NULL, NULL);
    return hw_timer;
}

void register_hw_timer(struct hw_timer *timer, struct timespec resoltuion) {
    timer->resolution = resoltuion;
    list_append(&s_hw_timers, &timer->list);
}

struct hw_timer *hw_primary_timer(void) {
    return list_first_entry(&s_hw_timers, struct hw_timer, list);
}

struct list_node *hw_timers(void) {
    return &s_hw_timers;
}
