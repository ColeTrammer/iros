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

static void pit_setup_interval_timer(struct hw_timer *self, int channel_index, long frequency, int irq_flags,
                                     hw_timer_callback_t callback) {
    uint16_t divisor = PIT_GET_DIVISOR(frequency);

    struct hw_timer_channel *channel = &self->channels[channel_index];
    assert(!channel->valid);

    init_hw_timer_channel(channel, handle_pit_interrupt, IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_NO_EOI | irq_flags, self, HW_TIMER_INTERVAL,
                          PIT_GET_FREQUENCY(divisor), callback);
    register_irq_handler(&channel->irq_handler, PIT_IRQ_LINE + EXTERNAL_IRQ_OFFSET);

    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_SQUARE_WAVE);
    outb(PIT_CHANNEL_0, divisor & 0xFF);
    outb(PIT_CHANNEL_0, divisor >> 8);
}

static void pit_setup_one_shot_timer(struct hw_timer *self, int channel_index, struct timespec delay, hw_timer_callback_t callback) {
    unsigned long ticks = time_divide(delay, PIT_BASE_RATE);
    assert(ticks <= UINT16_MAX);

    struct hw_timer_channel *channel = &self->channels[channel_index];
    assert(!channel->valid);

    init_hw_timer_channel(channel, handle_pit_interrupt, IRQ_HANDLER_EXTERNAL | IRQ_HANDLER_NO_EOI, self, HW_TIMER_SINGLE_SHOT, 0,
                          callback);
    register_irq_handler(&channel->irq_handler, PIT_IRQ_LINE + EXTERNAL_IRQ_OFFSET);

    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_INTERRUPT);
    outb(PIT_CHANNEL_0, ticks & 0xFF);
    outb(PIT_CHANNEL_0, (ticks >> 8) & 0xFF);
}

static void pit_disable_channel(struct hw_timer *self, int channel_index) {
    struct hw_timer_channel *channel = &self->channels[channel_index];

    uint64_t save = disable_interrupts_save();
    unregister_irq_handler(&channel->irq_handler, PIT_IRQ_LINE + EXTERNAL_IRQ_OFFSET);

    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_INTERRUPT);
    outb(PIT_CHANNEL_0, 0);
    outb(PIT_CHANNEL_0, 0);

    interrupts_restore(save);
    destroy_hw_timer_channel(channel);
}

static struct hw_timer_ops pit_ops = {
    .setup_interval_timer = &pit_setup_interval_timer,
    .setup_one_shot_timer = &pit_setup_one_shot_timer,
    .disable_channel = &pit_disable_channel,
};

static void detect_pit(struct hw_device *parent) {
    struct hw_timer *device =
        create_hw_timer("PIT", parent, hw_device_id_isa(), HW_TIMER_INTERVAL | HW_TIMER_SINGLE_SHOT, PIT_BASE_RATE, &pit_ops, 1);
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
