#include <stdio.h>

#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/output.h>

static void (*callback)(void) = NULL;

static void handle_pit_interrupt() {
    if (callback != NULL) {
        callback();
    }
}

void pit_register_callback(void (*_callback)(void)) {
    callback = _callback;
}

void pit_set_rate(unsigned int rate) {
    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_SQUARE_WAVE);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(rate) & 0xFF);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(rate) >> 8);
}

void init_pit() {
    register_irq_line_handler(&handle_pit_interrupt, PIT_IRQ_LINE);

    pit_set_rate(1);
}