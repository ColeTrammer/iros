#include <stdio.h>

#include <kernel/arch/x86_64/proc/process.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/hal/x86_64/drivers/pit.h>
#include <kernel/hal/output.h>

static void (*callback)(struct process_state*) = NULL;

static unsigned int count = 0;
static unsigned int count_to = 0;

void handle_pit_interrupt(struct process_state *process_state) {
    sendEOI(PIT_IRQ_LINE);
    
    if (callback != NULL) {
        count++;
        if (count >= count_to) {
            count = 0;
            callback(process_state);
        }
    }
}

void pit_register_callback(void (*_callback)(struct process_state*), unsigned int ms) {
    callback = _callback;
    count_to = ms;
}

void pit_set_rate(unsigned int rate) {
    PIT_SET_MODE(0, PIT_ACCESS_LOHI, PIT_MODE_SQUARE_WAVE);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(rate) & 0xFF);
    outb(PIT_CHANNEL_0, PIT_GET_DIVISOR(rate) >> 8);
}

void init_pit() {
    register_irq_line_handler(&handle_pit_interrupt_entry, PIT_IRQ_LINE, false);

    pit_set_rate(1);
}