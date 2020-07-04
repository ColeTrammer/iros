#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kernel/irqs/handlers.h>

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/fdc.h>
#include <kernel/hal/x86_64/drivers/pic.h>

static volatile bool has_been_interrupted = false;

#define wait_for_interrupt()      \
    has_been_interrupted = false; \
    while (!has_been_interrupted) \
        ;

static void handle_fdc_interrupt(struct irq_context *context __attribute__((unused))) {
    has_been_interrupted = true;
}

static struct irq_handler fdc_handler = { .handler = &handle_fdc_interrupt, .flags = IRQ_HANDLER_EXTERNAL };

void init_fdc() {
    register_irq_handler(&fdc_handler, FDC_IRQ_LINE + PIC_IRQ_OFFSET);
}
