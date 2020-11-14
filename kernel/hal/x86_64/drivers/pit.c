#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#include <kernel/hal/hw_device.h>
#include <kernel/hal/hw_timer.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/processor.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/irqs/handlers.h>
#include <kernel/proc/profile.h>
#include <kernel/proc/task.h>
#include <kernel/time/clock.h>
#include <kernel/util/init.h>

bool handle_pit_interrupt(struct irq_context *context) {
    context->irq_controller->ops->send_eoi(context->irq_controller, context->irq_num);

    struct hw_timer_channel *channel = context->closure;
    if (channel->callback) {
        channel->callback(channel, context);
    }
    return true;
}

static void pit_setup_interval_timer(struct hw_timer *self, int channel_index, hw_timer_callback_t callback) {
    struct hw_timer_channel *channel = &self->channels[channel_index];
    assert(!channel->valid);

    init_hw_timer_channel(channel, handle_pit_interrupt, IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_ALL_CPUS, self, HW_TIMER_INTERVAL,
                          (struct timespec) { .tv_nsec = 1000000 }, callback);
    register_irq_handler(&channel->irq_handler, PIT_IRQ_LINE + EXTERNAL_IRQ_OFFSET);

    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_SQUARE_WAVE);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(1) & 0xFF);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(1) >> 8);
}

static struct hw_timer_ops pit_ops = {
    .setup_interval_timer = &pit_setup_interval_timer,
};

static void detect_pit(struct hw_device *parent) {
    struct hw_timer *device = create_hw_timer("PIT", parent, hw_device_id_isa(), HW_TIMER_INTERVAL | HW_TIMER_SINGLE_SHOT,
                                              (struct timespec) { .tv_nsec = 100000 }, &pit_ops, 1);
    device->hw_device.status = HW_STATUS_ACTIVE;
    register_hw_timer(device);
}

static struct isa_driver pit_driver = {
    .name = "x86 PIT",
    .detect_devices = detect_pit,
};

static void init_pit(void) {
    register_isa_driver(&pit_driver);
}
INIT_FUNCTION(init_pit, driver);
